#pragma once

#include "ashura/gfx.h"
#include "stx/span.h"

namespace ash
{
namespace rhi
{

constexpr u8 MAX_HEAP_PROPERTIES = 32;
constexpr u8 MAX_HEAPS           = 16;

enum class DeviceType : u8
{
  Other         = 0,
  IntegratedGpu = 1,
  DiscreteGpu   = 2,
  VirtualGpu    = 3,
  Cpu           = 4
};

/// [properties] is either of:
///
/// HostVisible | HostCoherent
/// HostVisible | HostCached
/// HostVisible | HostCached | HostCoherent
/// DeviceLocal
/// DeviceLocal | HostVisible | HostCoherent
/// DeviceLocal | HostVisible | HostCached
/// DeviceLocal | HostVisible | HostCached | HostCoherent
struct HeapProperty
{
  gfx::MemoryProperties properties = gfx::MemoryProperties::None;
  u32                   index      = 0;
};

// TODO(lamarrr): write memory allocation strategies, i.e. images should be allocated on this and
// this heap a single heap might have multiple properties
struct DeviceMemoryHeaps
{
  // ordered by performance-tier (MemoryProperties)
  HeapProperty heap_properties[MAX_HEAP_PROPERTIES] = {};
  u8           num_properties                       = 0;
  u64          heap_sizes[MAX_HEAPS]                = {};
  u8           num_heaps                            = 0;

  constexpr bool has_memory(gfx::MemoryProperties properties) const
  {
    for (u8 i = 0; i < num_properties; i++)
    {
      if (has_bits(heap_properties[i].properties, properties))
      {
        return true;
      }
    }
    return false;
  }

  constexpr bool has_unified_memory() const
  {
    return has_memory(gfx::MemoryProperties::DeviceLocal | gfx::MemoryProperties::HostVisible);
  }
};

// TODO(lamarrr): formats info properties
// device selection
// instance
struct DeviceInfo
{
  DeviceType        type                = DeviceType::Other;
  DeviceMemoryHeaps memory_heaps        = {};
  f32               max_anisotropy      = 1.0f;
  bool              supports_raytracing = false;
  // device type
  // device name
  // vendor name
  // driver name
  // current display size
  // current display format
  // supports hdr?
  // supports video encode
  // supports video decode
  // is format hdr
  // dci p3?
  //
};

struct Driver
{
  virtual ~Driver()                                                                        = 0;
  virtual gfx::FormatProperties    get_format_properties(gfx::Format format)               = 0;
  virtual gfx::Buffer              create(gfx::BufferDesc const &desc)                     = 0;
  virtual gfx::BufferView          create(gfx::BufferViewDesc const &desc)                 = 0;
  virtual gfx::Image               create(gfx::ImageDesc const &desc)                      = 0;
  virtual gfx::ImageView           create(gfx::ImageViewDesc const &desc)                  = 0;
  virtual gfx::RenderPass          create(gfx::RenderPassDesc const &desc)                 = 0;
  virtual gfx::Framebuffer         create(gfx::FramebufferDesc const &desc)                = 0;
  virtual gfx::Sampler             create(gfx::SamplerDesc const &sampler)                 = 0;
  virtual gfx::DescriptorSetLayout create(gfx::DescriptorSetLayoutDesc const &desc)        = 0;
  virtual gfx::ComputePipeline     create(gfx::ComputePipelineDesc const &desc)            = 0;
  virtual gfx::GraphicsPipeline    create(gfx::GraphicsPipelineDesc const &desc)           = 0;
  virtual gfx::CommandBuffer       create_command_buffer()                                 = 0;
  virtual gfx::Shader              create_shader(stx::Span<u32 const> shader)              = 0;
  virtual void                     destroy(gfx::Buffer buffer)                             = 0;
  virtual void                     destroy(gfx::BufferView buffer_view)                    = 0;
  virtual void                     destroy(gfx::Image image)                               = 0;
  virtual void                     destroy(gfx::ImageView image_view)                      = 0;
  virtual void                     destroy(gfx::RenderPass render_pass)                    = 0;
  virtual void                     destroy(gfx::Framebuffer framebuffer)                   = 0;
  virtual void                     destroy(gfx::Sampler sampler)                           = 0;
  virtual void                     destroy(gfx::DescriptorSetLayout descriptor_set_layout) = 0;
  virtual void                     destroy(gfx::Shader shader)                             = 0;
  virtual void                     destroy(gfx::ComputePipeline pipeline)                  = 0;
  virtual void                     destroy(gfx::GraphicsPipeline pipeline)                 = 0;
  virtual void                     destroy(gfx::CommandBuffer command_buffer)              = 0;
  virtual void                     wait_idle()                                             = 0;
  virtual void                     wait_for_complete(gfx::CommandBuffer command_buffer)    = 0;
  virtual void                     submit(gfx::CommandBuffer command_buffer);
  virtual void cmd_fill_buffer(gfx::CommandBuffer command_buffer, gfx::Buffer dst, u64 offset,
                               u64 size, u32 data)                                       = 0;
  virtual void cmd_copy_buffer(gfx::CommandBuffer command_buffer, gfx::Buffer src, gfx::Buffer dst,
                               stx::Span<gfx::BufferCopy const> copies)                  = 0;
  virtual void cmd_update_buffer(gfx::CommandBuffer command_buffer, stx::Span<u8 const> src,
                                 u64 dst_offset, gfx::Buffer dst)                        = 0;
  virtual void cmd_clear_color_image(gfx::CommandBuffer command_buffer, gfx::Image dst,
                                     stx::Span<gfx::Color const>                 clear_colors,
                                     stx::Span<gfx::ImageSubresourceRange const> ranges) = 0;
  virtual void
               cmd_clear_depth_stencil_image(gfx::CommandBuffer command_buffer, gfx::Image dst,
                                             stx::Span<gfx::DepthStencil const> clear_depth_stencils,
                                             stx::Span<gfx::ImageSubresourceRange const> ranges) = 0;
  virtual void cmd_copy_image(gfx::CommandBuffer command_buffer, gfx::Image src, gfx::Image dst,
                              stx::Span<gfx::ImageCopy const> copies)                    = 0;
  virtual void cmd_copy_buffer_to_image(gfx::CommandBuffer command_buffer, gfx::Buffer src,
                                        gfx::Image                            dst,
                                        stx::Span<gfx::BufferImageCopy const> copies)    = 0;
  virtual void cmd_blit_image(gfx::CommandBuffer command_buffer, gfx::Image src, gfx::Image dst,
                              stx::Span<gfx::ImageBlit const> blits, gfx::Filter filter) = 0;
  virtual void cmd_begin_render_pass(
      gfx::CommandBuffer command_buffer, gfx::Framebuffer framebuffer, gfx::RenderPass render_pass,
      IRect render_area, stx::Span<gfx::Color const> color_attachments_clear_values,
      stx::Span<gfx::DepthStencil const> depth_stencil_attachments_clear_values) = 0;
  virtual void cmd_end_render_pass(gfx::CommandBuffer command_buffer)            = 0;
  virtual void cmd_dispatch(gfx::CommandBuffer command_buffer, gfx::ComputePipeline pipeline,
                            u32 group_count_x, u32 group_count_y, u32 group_count_z,
                            gfx::DescriptorSetBindings const &bindings,
                            stx::Span<u8 const>               push_constants_data)             = 0;
  virtual void cmd_dispatch_indirect(gfx::CommandBuffer   command_buffer,
                                     gfx::ComputePipeline pipeline, gfx::Buffer buffer, u64 offset,
                                     gfx::DescriptorSetBindings const &bindings,
                                     stx::Span<u8 const>               push_constants_data)    = 0;
  virtual void cmd_draw(gfx::CommandBuffer command_buffer, gfx::GraphicsPipeline pipeline,
                        gfx::RenderState const &state, stx::Span<gfx::Buffer const> vertex_buffers,
                        gfx::Buffer index_buffer, u32 first_index, u32 num_indices,
                        u32 vertex_offset, u32 first_instance, u32 num_instances,
                        gfx::DescriptorSetBindings const &bindings,
                        stx::Span<u8 const>               push_constants_data)                 = 0;
  virtual void cmd_draw_indirect(gfx::CommandBuffer command_buffer, gfx::GraphicsPipeline pipeline,
                                 gfx::RenderState const      &state,
                                 stx::Span<gfx::Buffer const> vertex_buffers,
                                 gfx::Buffer index_buffer, gfx::Buffer buffer, u64 offset,
                                 u32 draw_count, u32 stride,
                                 gfx::DescriptorSetBindings const &bindings,
                                 stx::Span<u8 const>               push_constants_data)        = 0;
  virtual void
      cmd_insert_barriers(gfx::CommandBuffer                        command_buffer,
                          stx::Span<gfx::BufferMemoryBarrier const> buffer_memory_barriers,
                          stx::Span<gfx::ImageMemoryBarrier const>  image_memory_barriers) = 0;
};

};        // namespace rhi
}        // namespace ash
