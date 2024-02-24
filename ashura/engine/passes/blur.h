#pragma once
#include "ashura/engine/renderer.h"

namespace ash
{
// TODO(lamarrr): should this be less general-purpose?
// do we need to copy directly from the image?
enum class BlurRadius : u8
{
  None     = 0,
  Radius1  = 1,
  Radius2  = 2,
  Radius4  = 4,
  Radius8  = 8,
  Radius16 = 16,
  Radius32 = 32
};

struct BlurParams
{
  BlurRadius blur_radius = BlurRadius::None;
  rdg::Attachment src         = {};
  rdg::Attachment dst         = {};
};

// object-clip space blur
//
// - capture scene at object's screen-space area, dilate by the blur extent
// - reserve scratch stencil image with at least size of the dilated area
// - blur captured area
// - render object to offscreen scratch image stencil only
// - using rendered stencil, directly-write (without blending) onto scene again
struct BlurPass
{
  static void init(Pass self, RenderServer *server, uid32 id);
  static void deinit(Pass self, RenderServer *server);

  static constexpr PassInterface const interface{.init   = init,
                                                 .deinit = deinit};

  void execute(rdg::RenderGraph *graph, BlurParams const *params);
};

}        // namespace ash
