/// SPDX-License-Identifier: MIT
#include "ashura/engine/renderer.h"
#include "ashura/engine/canvas.h"
#include "ashura/std/math.h"
#include "ashura/std/range.h"

namespace ash
{

void Renderer::begin_frame(GpuContext &ctx, RenderTarget const &rt)
{
  gpu::CommandEncoderImpl enc = ctx.encoder();
  Resources              &r   = resources[ctx.ring_index()];

  canvas->end_recording();

  r.ngon_vertices.copy(ctx, span(canvas->ngon_vertices).as_u8());
  r.ngon_indices.copy(ctx, span(canvas->ngon_indices).as_u8());
  r.ngon_params.copy(ctx, span(canvas->ngon_params).as_u8());
  r.rrect_params.copy(ctx, span(canvas->rrect_params).as_u8());
  r.ngon_vertices.copy(ctx, span(canvas->ngon_vertices).as_u8());

  for (auto &p : pipelines)
  {
    p->begin_frame(ctx, passes, enc);
  }
}

void Renderer::end_frame(GpuContext &ctx, RenderTarget const &)
{
  gpu::CommandEncoderImpl enc = ctx.encoder();

  for (auto &p : pipelines)
  {
    p->end_frame(ctx, passes, enc);
  }
}

void Renderer::render_frame(GpuContext &ctx, RenderTarget const &rt)
{
  Resources              &r       = resources[ctx.ring_index()];
  gpu::CommandEncoderImpl encoder = ctx.encoder();

  Canvas::RenderContext render_ctx{.canvas        = *canvas,
                                   .gpu           = ctx,
                                   .passes        = passes,
                                   .rt            = rt,
                                   .enc           = encoder,
                                   .rrects        = r.rrect_params,
                                   .ngons         = r.ngon_params,
                                   .ngon_vertices = r.ngon_vertices,
                                   .ngon_indices  = r.ngon_indices};

  for (Canvas::Pass const &pass : canvas->passes)
  {
    pass.task(render_ctx);
  }
}

}        // namespace ash