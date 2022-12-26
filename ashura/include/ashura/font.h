#pragma once
#include <filesystem>
#include <fstream>
#include <string_view>

#include "ashura/image.h"
#include "ashura/primitives.h"
#include "ashura/vulkan.h"
#include "freetype/freetype.h"
#include "hb.h"
#include "stx/allocator.h"
#include "stx/limits.h"
#include "stx/span.h"
#include "stx/string.h"
#include "stx/vec.h"

namespace asr {

enum class TextAlign : u8 { Left, Right, Center };

enum class WordWrap : u8 { None, Wrap };

struct TextStyle {
  f32 font_height = 16;
  f32 letter_spacing = 2;
  u32 tab_size = 8;
  f32 word_spacing = 4;
  // multiplied by font_height
  f32 line_height = 1.2f;
  hb_direction_t direction = HB_DIRECTION_LTR;
  TextAlign align = TextAlign::Left;
  // WordWrap word_wrap = WordWrap::None;
  bool use_kerning = true;
  //  use standard and contextual ligature substitution
  bool use_ligatures = true;
};

struct Font {
  static constexpr hb_tag_t KERNING_FEATURE =
      HB_TAG('k', 'e', 'r', 'n');  // kerning operations
  static constexpr hb_tag_t LIGATURE_FEATURE =
      HB_TAG('l', 'i', 'g', 'a');  // standard ligature substitution
  static constexpr hb_tag_t CONTEXTUAL_LIGATURE_FEATURE =
      HB_TAG('c', 'l', 'i', 'g');  // contextual ligature substitution

  hb_face_t* hbface = nullptr;
  hb_font_t* hbfont = nullptr;
  hb_buffer_t* hbscratch_buffer = nullptr;
  FT_Library ftlib = nullptr;
  FT_Face ftface = nullptr;
  stx::Memory font_data;

  Font(hb_face_t* ahbface, hb_font_t* ahbfont, hb_buffer_t* ahbscratch_buffer,
       FT_Library aftlib, FT_Face aftface, stx::Memory afont_data)
      : hbface{ahbface},
        hbfont{ahbfont},
        hbscratch_buffer{ahbscratch_buffer},
        ftlib{aftlib},
        ftface{aftface},
        font_data{std::move(afont_data)} {}

  STX_MAKE_PINNED(Font)

  ~Font() {
    hb_face_destroy(hbface);
    hb_font_destroy(hbfont);
    hb_buffer_destroy(hbscratch_buffer);
    ASR_CHECK(FT_Done_Face(ftface) == 0);
    ASR_CHECK(FT_Done_FreeType(ftlib) == 0);
  }
};

enum class FontLoadError { InvalidPath };

inline stx::Rc<Font*> load_font_from_memory(stx::Memory memory, usize size) {
  std::cout << "size: " << size << std::endl;
  hb_blob_t* hbblob = hb_blob_create(static_cast<char*>(memory.handle),
                                     static_cast<unsigned int>(size),
                                     HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  ASR_CHECK(hbblob != nullptr);

  hb_face_t* hbface = hb_face_create(hbblob, 0);
  ASR_CHECK(hbface != nullptr);

  hb_font_t* hbfont = hb_font_create(hbface);
  ASR_CHECK(hbfont != nullptr);

  hb_buffer_t* hbscratch_buffer = hb_buffer_create();
  ASR_CHECK(hbscratch_buffer != nullptr);

  FT_Library ftlib;
  ASR_CHECK(FT_Init_FreeType(&ftlib) == 0);

  FT_Face ftface;
  ASR_CHECK(FT_New_Memory_Face(ftlib,
                               static_cast<FT_Byte const*>(memory.handle),
                               static_cast<FT_Long>(size), 0, &ftface) == 0);

  return stx::rc::make_inplace<Font>(stx::os_allocator, hbface, hbfont,
                                     hbscratch_buffer, ftlib, ftface,
                                     std::move(memory))
      .unwrap();
}

inline stx::Result<stx::Rc<Font*>, FontLoadError> load_font_from_file(
    stx::String const& path) {
  if (!std::filesystem::exists(path.c_str())) {
    return stx::Err(FontLoadError::InvalidPath);
  }

  std::ifstream stream{path.c_str(), std::ios::ate | std::ios_base::binary};

  usize size = stream.tellg();
  stream.seekg(0);

  stx::Memory memory = stx::mem::allocate(stx::os_allocator, size).unwrap();

  stream.read(static_cast<char*>(memory.handle), size);

  stream.close();

  return stx::Ok(load_font_from_memory(std::move(memory), size));
};

struct Glyph {
  // the glyph index
  u32 index = 0;
  // unicode codepoint
  u32 codepoint = 0;
  // offset into the atlas its glyph resides
  asr::offset offset;
  // extent of the glyph in the atlas
  extent extent;
  // defines offset from cursor position the glyph will be placed
  vec2 pos;
  // advancement of the cursor after drawing this glyph
  vec2 advance;
  // texture coordinates of this glyph in the atlas
  f32 s0 = 0, t0 = 0, s1 = 0, t1 = 0;
};

// stores codepoint glyphs for a font at a specific font height
// this is a single-column atlas
struct FontAtlas {
  // indices begin from 1
  stx::Vec<Glyph> glyphs{stx::os_allocator};
  // overall extent of the atlas
  extent extent;
  // font height at which the cache/atlas/glyphs will be rendered and cached
  u32 font_height = 40;
  // atlas containing the glyphs
  Image atlas;

  stx::Span<Glyph> get(u32 index) const {
    if (index < 1 || index > glyphs.size()) return {};
    return glyphs.span().slice(index - 1, 1);
  }
};

inline FontAtlas render_font(Font& font, vk::RecordingContext& ctx,
                             u32 font_height) {
  // *64 to convert font height to 26.6 pixel format
  ASR_CHECK(font_height > 0);
  ASR_CHECK(FT_Set_Char_Size(font.ftface, 0, font_height * 64, 72, 72) == 0);

  vk::CommandQueue& cqueue = *ctx.queue.value().handle;
  VkDevice dev = cqueue.device.handle->device;
  VkPhysicalDeviceMemoryProperties const& memory_properties =
      cqueue.device.handle->phy_device.handle->memory_properties;

  stx::Vec<Glyph> glyphs{stx::os_allocator};

  // we use column layout because writing the rendered glyphs to it will
  // be faster and loading from memory during rendering will be faster as
  // opposed to laying them all out on one row which will be extremely slow as
  // unrelated pixel rows will be loaded into memory during rendering leading to
  // very high cache misses.

  {
    FT_UInt glyph_index;

    FT_ULong codepoint = FT_Get_First_Char(font.ftface, &glyph_index);

    while (glyph_index != 0) {
      std::cout << "GOT CHAR: " << codepoint << " i=" << glyph_index << std::endl;
      if (FT_Load_Glyph(font.ftface, codepoint, 0) == 0) {
        u32 width = font.ftface->glyph->bitmap.width;
        u32 height = font.ftface->glyph->bitmap.rows;

        vec2 pos{AS_F32(font.ftface->glyph->bitmap_left),
                 AS_F32(font_height - font.ftface->glyph->bitmap_top)};

        // convert from 26.6 pixel format
        vec2 advance{font.ftface->glyph->advance.x / 64.0f,
                     font.ftface->glyph->advance.y / 64.0f};

        glyphs
            .push(Glyph{.index = glyph_index,
                        .codepoint = codepoint,
                        .offset = {},
                        .extent = {width, height},
                        .pos = pos,
                        .advance = advance,
                        .s0 = 0,
                        .t0 = 0,
                        .s1 = 0,
                        .t1 = 0})
            .unwrap();
      }
      codepoint = FT_Get_Next_Char(font.ftface, codepoint, &glyph_index);
    }
  }

  glyphs.span().sort(
      [](Glyph const& a, Glyph const& b) { return a.index < b.index; });

  extent atlas_extent;

  // TODO(lamarrr): some glyphs don't exist, we might need to add a new bool to
  // specify if it is valid or not? or else we'll have to horribly perform
  // lookup across all the glyphs which is costly

  // now that we know the full atlas extent calculate texture coordinates
  for (Glyph& glyph : glyphs) {
    // std::cout << "codepoint: " << codepoint << " w: " << width
    //         << " h:" << height << std::endl;
    offset offset{0, atlas_extent.height};
    atlas_extent.width = std::max(atlas_extent.width, glyph.extent.width);
    atlas_extent.height += glyph.extent.height + 1;

    glyph.s0 = AS_F32(glyph.offset.x) / atlas_extent.width;
    glyph.s1 = AS_F32(glyph.offset.x + glyph.extent.width) / atlas_extent.width;
    glyph.t0 = AS_F32(glyph.offset.y) / atlas_extent.height;
    glyph.t1 =
        AS_F32(glyph.offset.y + glyph.extent.height) / atlas_extent.height;

    std::cout << "glyph: " << glyph.index << std::endl;
  }

  // TODO-future(lamarrr): we should probably handle this instead of bailing out
  // NOTE: vulkan doesn't allow zero-extent images
  atlas_extent.width = std::max<u32>(1, atlas_extent.width);
  atlas_extent.height = std::max<u32>(1, atlas_extent.height);

  vk::Buffer cache_staging_buffer =
      vk::create_host_buffer(dev, memory_properties, atlas_extent.area() * 4,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  u8* out = static_cast<u8*>(cache_staging_buffer.memory_map);

  for (Glyph const& glyph : glyphs) {
    ASR_CHECK(FT_Load_Glyph(font.ftface, glyph.codepoint, 0) == 0);
    ASR_CHECK(FT_Render_Glyph(font.ftface->glyph, FT_RENDER_MODE_NORMAL) == 0);

    for (usize j = 0; j < glyph.extent.height; j++) {
      // copy the rendered glyph to the atlas
      for (usize i = 0; i < glyph.extent.width; i++) {
        out[0] = 0xFF;
        out[1] = 0xFF;
        out[2] = 0xFF;
        out[3] = font.ftface->glyph->bitmap.buffer[j * glyph.extent.width + i];
        out += 4;
      }
      // fill the unused portion of the row with transparent pixels
      for (usize i = 0; i < (atlas_extent.width - glyph.extent.width); i++) {
        out[0] = 0xFF;
        out[1] = 0xFF;
        out[2] = 0xFF;
        out[3] = 0x00;
        out += 4;
      }
    }

    for (usize i = 0; i < atlas_extent.width; i++) {
      out[0] = 0xFF;
      out[1] = 0xFF;
      out[2] = 0xFF;
      out[3] = 0x00;
      out += 4;
    }
  }

  VkImageCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .extent = VkExtent3D{.width = atlas_extent.width,
                           .height = atlas_extent.height,
                           .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  VkImage image;

  ASR_VK_CHECK(vkCreateImage(dev, &create_info, nullptr, &image));

  VkMemoryRequirements memory_requirements;

  vkGetImageMemoryRequirements(dev, image, &memory_requirements);

  u32 memory_type_index =
      vk::find_suitable_memory_type(memory_properties, memory_requirements,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
          .unwrap();

  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index};

  VkDeviceMemory memory;

  ASR_VK_CHECK(vkAllocateMemory(dev, &alloc_info, nullptr, &memory));

  ASR_VK_CHECK(vkBindImageMemory(dev, image, memory, 0));

  VkImageViewCreateInfo view_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .components = VkComponentMapping{.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange =
          VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1}};

  VkImageView view;

  ASR_VK_CHECK(vkCreateImageView(dev, &view_create_info, nullptr, &view));

  VkCommandBufferBeginInfo cmd_buffer_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr};

  ASR_VK_CHECK(
      vkBeginCommandBuffer(ctx.upload_cmd_buffer, &cmd_buffer_begin_info));

  VkImageMemoryBarrier pre_upload_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_NONE_KHR,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1}};

  vkCmdPipelineBarrier(ctx.upload_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1,
                       &pre_upload_barrier);

  VkBufferImageCopy copy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          VkImageSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                   .mipLevel = 0,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1},
      .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
      .imageExtent = VkExtent3D{.width = atlas_extent.width,
                                .height = atlas_extent.height,
                                .depth = 1}};

  vkCmdCopyBufferToImage(ctx.upload_cmd_buffer, cache_staging_buffer.buffer,
                         image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

  VkImageMemoryBarrier post_upload_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1}};

  vkCmdPipelineBarrier(ctx.upload_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1,
                       &post_upload_barrier);

  ASR_VK_CHECK(vkEndCommandBuffer(ctx.upload_cmd_buffer));

  VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           .pNext = nullptr,
                           .waitSemaphoreCount = 0,
                           .pWaitSemaphores = nullptr,
                           .pWaitDstStageMask = nullptr,
                           .commandBufferCount = 1,
                           .pCommandBuffers = &ctx.upload_cmd_buffer,
                           .signalSemaphoreCount = 0,
                           .pSignalSemaphores = nullptr};

  ASR_VK_CHECK(vkResetFences(dev, 1, &ctx.upload_fence));

  ASR_VK_CHECK(
      vkQueueSubmit(cqueue.info.queue, 1, &submit_info, ctx.upload_fence));

  ASR_VK_CHECK(
      vkWaitForFences(dev, 1, &ctx.upload_fence, VK_TRUE, COMMAND_TIMEOUT));

  ASR_VK_CHECK(vkResetCommandBuffer(ctx.upload_cmd_buffer, 0));

  cache_staging_buffer.destroy(dev);

  return FontAtlas{
      .glyphs = std::move(glyphs),
      .extent = atlas_extent,
      .font_height = font_height,
      .atlas = vk::create_image_sampler(
          stx::rc::make_inplace<vk::ImageResource>(
              stx::os_allocator, image, view, memory, ctx.queue.value().share())
              .unwrap())};
}

// TODO(lamarrr): implement
template <typename Fn>
rect layout_text(f32 max_width, Fn& callback);

}  // namespace asr
