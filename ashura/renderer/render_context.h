#pragma once
#include "ashura/gfx/gfx.h"
#include "ashura/renderer/scene.h"
#include "ashura/renderer/shader.h"
#include "ashura/renderer/view.h"
#include "ashura/std/option.h"
#include "ashura/std/result.h"
#include "ashura/std/types.h"

namespace ash
{

struct Mesh
{
  gfx::Buffer    vertex_buffer        = 0;
  u64            vertex_buffer_offset = 0;
  gfx::Buffer    index_buffer         = 0;
  u64            index_buffer_offset  = 0;
  gfx::IndexType index_type           = gfx::IndexType::Uint16;
};

/// @color_images, @depth_stencil_image format must be same as render context's
struct RenderTarget
{
  Span<gfx::ImageView const> color_images;
  gfx::ImageView             depth_stencil_image   = nullptr;
  gfx::ImageAspects          depth_stencil_aspects = gfx::ImageAspects::None;
  Vec2U                      extent                = {};
  Vec2U                      render_offset         = {};
  Vec2U                      render_extent         = {};
};

/// with views for each image mip level
//
// created with sampled, storage, color attachment, and transfer flags
//
struct Scratch
{
  gfx::ImageDesc     color_image_desc              = {};
  gfx::ImageDesc     depth_stencil_image_desc      = {};
  gfx::ImageViewDesc color_image_view_desc         = {};
  gfx::ImageViewDesc depth_stencil_image_view_desc = {};
  gfx::Image         color_image                   = nullptr;
  gfx::Image         depth_stencil_image           = nullptr;
  gfx::ImageView     color_image_view              = {};
  gfx::ImageView     depth_stencil_image_view      = {};
};

/// @color_format: hdr if hdr supported and required
/// scratch images resized when swapchain extents changes
struct RenderContext
{
  gfx::DeviceImpl          device               = {};
  gfx::PipelineCache       pipeline_cache       = nullptr;
  gfx::FrameContext        frame_context        = nullptr;
  gfx::Swapchain           swapchain            = nullptr;
  gfx::FrameInfo           frame_info           = {};
  gfx::SwapchainInfo       swapchain_info       = {};
  gfx::Format              color_format         = gfx::Format::Undefined;
  gfx::Format              depth_stencil_format = gfx::Format::Undefined;
  Scratch                  scatch               = {};
  Vec<UniformHeap>         uniform_heaps        = {};
  gfx::DescriptorSetLayout uniform_layout       = nullptr;
  Vec<Tuple<gfx::FrameId, gfx::Framebuffer>> released_framebuffers = {};
  Vec<Tuple<gfx::FrameId, gfx::Image>>       released_images       = {};
  Vec<Tuple<gfx::FrameId, gfx::ImageView>>   released_image_views  = {};

  gfx::CommandEncoderImpl encoder() const;
  u32                     ring_index() const;
  u32                     num_frames_in_flight() const;
  template <typename T>
  Uniform push_uniform(T const &uniform);
  template <typename T>
  Uniform push_uniform_range(Span<T const> uniform);

  Option<gfx::Shader> get_shader(Span<char const> name);

  void release(gfx::Framebuffer framebuffer)
  {
    ENSURE(released_framebuffers.push(frame_info.current, framebuffer));
  }

  void release(gfx::Image image)
  {
    ENSURE(released_images.push(frame_info.current, image));
  }

  void release(gfx::ImageView view)
  {
    ENSURE(released_image_views.push(frame_info.current, view));
  }
};

}        // namespace ash
