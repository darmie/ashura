/// SPDX-License-Identifier: MIT
#include "ashura/engine/canvas.h"
#include "ashura/engine/font_atlas.h"
#include "ashura/std/math.h"

namespace ash
{

void path::rect(Vec<Vec2> &vtx)
{
  vtx.extend_copy(span<Vec2>({{-1, -1}, {1, -1}, {1, 1}, {-1, 1}})).unwrap();
}

void path::arc(Vec<Vec2> &vtx, f32 start, f32 stop, u32 segments)
{
  if (segments < 2)
  {
    return;
  }

  u32 const first = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  f32 const step = (stop - start) / (segments - 1);

  for (u32 i = 0; i < segments; i++)
  {
    vtx[first + i] = rotor(i * step) * 2 - 1;
  }
}

void path::circle(Vec<Vec2> &vtx, u32 segments)
{
  if (segments < 4)
  {
    return;
  }

  u32 const first = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  f32 const step = (2 * PI) / (segments - 1);

  for (u32 i = 0; i < segments; i++)
  {
    vtx[first + i] = rotor(i * step);
  }
}

void path::squircle(Vec<Vec2> &vtx, f32 elasticity, u32 segments)
{
  if (segments < 128)
  {
    return;
  }

  elasticity = clamp(elasticity, 0.0F, 1.0F);

  path::cubic_bezier(vtx, {0, -1}, {elasticity, -1}, {1, -1}, {1, 0},
                     segments >> 2);
  path::cubic_bezier(vtx, {1, 0}, {1, elasticity}, {1, 1}, {0, 1},
                     segments >> 2);
  path::cubic_bezier(vtx, {0, 1}, {-elasticity, 1}, {-1, 1}, {-1, 0},
                     segments >> 2);
  path::cubic_bezier(vtx, {-1, 0}, {-1, -elasticity}, {-1, -1}, {0, -1},
                     segments >> 2);
}

void path::rrect(Vec<Vec2> &vtx, Vec4 radii, u32 segments)
{
  if (segments < 8)
  {
    return;
  }

  radii   = radii * 2;
  radii.x = min(radii.x, 2.0f);
  radii.y = min(radii.y, 2.0f);
  radii.z = min(radii.z, 2.0f);
  radii.w = min(radii.w, 2.0f);

  /// clipping
  radii.y          = min(radii.y, 2.0f - radii.x);
  f32 max_radius_z = min(2.0f - radii.x, 1.0f - radii.y);
  radii.z          = min(radii.z, max_radius_z);
  f32 max_radius_w = min(max_radius_z, 1.0f - radii.z);
  radii.w          = min(radii.w, max_radius_w);

  u32 const curve_segments = (segments - 8) / 4;
  f32 const step  = (curve_segments == 0) ? 0.0f : ((PI / 2) / curve_segments);
  u32 const first = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  u32 i = 0;

  vtx[first + i++] = Vec2{1, 1 - radii.z};

  for (u32 s = 0; s < curve_segments; s++)
  {
    vtx[first + i++] = (1 - radii.z) + radii.z * rotor(s * step);
  }

  vtx[first + i++] = Vec2{1 - radii.z, 1};

  vtx[first + i++] = Vec2{-1 + radii.w, 1};

  for (u32 s = 0; s < curve_segments; s++)
  {
    vtx[first + i++] =
        Vec2{-1 + radii.w, 1 - radii.w} + radii.w * rotor(PI / 2 + s * step);
  }

  vtx[first + i++] = Vec2{-1, 1 - radii.w};

  vtx[first + i++] = Vec2{-1, -1 + radii.x};

  for (u32 s = 0; s < curve_segments; s++)
  {
    vtx[first + i++] = (-1 + radii.x) + radii.x * rotor(PI + s * step);
  }

  vtx[first + i++] = Vec2{-1 + radii.x, -1};

  vtx[first + i++] = Vec2{1 - radii.y, -1};

  for (u32 s = 0; s < curve_segments; s++)
  {
    vtx[first + i++] = Vec2{1 - radii.y, (-1 + radii.y)} +
                       radii.y * rotor(PI * 3.0f / 2.0f + s * step);
  }

  vtx[first + i++] = Vec2{1, -1 + radii.y};
}

void path::brect(Vec<Vec2> &vtx, Vec4 slant)
{
  slant   = slant * 2.0f;
  slant.x = min(slant.x, 2.0f);
  slant.y = min(slant.y, 2.0f);
  slant.z = min(slant.z, 2.0f);
  slant.w = min(slant.w, 2.0f);

  slant.y          = min(slant.y, 2.0f - slant.x);
  f32 max_radius_z = min(2.0f - slant.x, 2.0f - slant.y);
  slant.z          = min(slant.z, max_radius_z);
  f32 max_radius_w = min(max_radius_z, 2.0f - slant.z);
  slant.w          = min(slant.w, max_radius_w);

  Vec2 const vertices[] = {{-1 + slant.x, -1}, {1 - slant.y, -1},
                           {1, -1 + slant.y},  {1, 1 - slant.z},
                           {1 - slant.z, 1},   {-1 + slant.w, 1},
                           {-1, 1 - slant.w},  {-1, -1 + slant.x}};

  vtx.extend_copy(span(vertices)).unwrap();
}

void path::bezier(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, u32 segments)
{
  if (segments < 3)
  {
    return;
  }

  u32 const first = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  f32 const step = 1.0f / (segments - 1);

  for (u32 i = 0; i < segments; i++)
  {
    vtx[first + i] = Vec2{ash::bezier(cp0.x, cp1.x, cp2.x, step * i),
                          ash::bezier(cp0.y, cp1.y, cp2.y, step * i)};
  }
}

void path::cubic_bezier(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, Vec2 cp3,
                        u32 segments)
{
  if (segments < 4)
  {
    return;
  }

  u32 const first = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  f32 const step = 1.0f / (segments - 1);

  for (u32 i = 0; i < segments; i++)
  {
    vtx[first + i] =
        Vec2{ash::cubic_bezier(cp0.x, cp1.x, cp2.x, cp3.x, step * i),
             ash::cubic_bezier(cp0.y, cp1.y, cp2.y, cp3.y, step * i)};
  }
}

void path::catmull_rom(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, Vec2 cp3,
                       u32 segments)
{
  if (segments < 4)
  {
    return;
  }

  u32 const beg = vtx.size32();

  vtx.extend_uninit(segments).unwrap();

  f32 const step = 1.0f / (segments - 1);

  for (u32 i = 0; i < segments; i++)
  {
    vtx[beg + i] =
        Vec2{ash::cubic_bezier(cp0.x, cp1.x, cp2.x, cp3.x, step * i),
             ash::cubic_bezier(cp0.y, cp1.y, cp2.y, cp3.y, step * i)};
  }
}

void path::triangulate_stroke(Span<Vec2 const> points, Vec<Vec2> &vertices,
                              Vec<u32> &indices, f32 thickness)
{
  if (points.size() < 2)
  {
    return;
  }

  u32 const first_vtx    = vertices.size32();
  u32 const first_idx    = indices.size32();
  u32 const num_points   = points.size32();
  u32 const num_vertices = (num_points - 1) * 4;
  u32 const num_indices  = (num_points - 1) * 6 + (num_points - 2) * 6;
  vertices.extend_uninit(num_vertices).unwrap();
  indices.extend_uninit(num_indices).unwrap();

  Vec2 *vtx  = vertices.data() + first_vtx;
  u32  *idx  = indices.data() + first_idx;
  u32   ivtx = 0;

  for (u32 i = 0; i < num_points - 1; i++)
  {
    Vec2 const p0    = points[i];
    Vec2 const p1    = points[i + 1];
    Vec2 const d     = p1 - p0;
    f32 const  alpha = atanf(d.y / d.x);

    // parallel angle
    Vec2 const down = (thickness / 2) * rotor(alpha + PI / 2);

    // perpendicular angle
    Vec2 const up = -down;

    vtx[0] = p0 + up;
    vtx[1] = p0 + down;
    vtx[2] = p1 + up;
    vtx[3] = p1 + down;
    idx[0] = ivtx;
    idx[1] = ivtx + 1;
    idx[2] = ivtx + 3;
    idx[3] = ivtx;
    idx[4] = ivtx + 3;
    idx[5] = ivtx + 2;

    if (i != 0)
    {
      u32 prev = ivtx - 2;
      idx[6]   = prev;
      idx[7]   = prev + 1;
      idx[8]   = ivtx + 1;
      idx[9]   = prev;
      idx[10]  = prev + 1;
      idx[11]  = ivtx;
      idx += 6;
    }

    idx += 6;
    vtx += 4;
    ivtx += 4;
  }
}

void path::triangles(u32 first_vertex, u32 num_vertices, Vec<u32> &indices)
{
  CHECK(num_vertices > 3);
  u32 const num_triangles = num_vertices / 3;
  u32 const first_idx     = indices.size32();
  indices.extend_uninit(num_triangles * 3).unwrap();

  u32 *idx = indices.data() + first_idx;
  for (u32 i = 0; i < num_triangles * 3; i += 3)
  {
    idx[i]     = first_vertex + i;
    idx[i + 1] = first_vertex + i + 1;
    idx[i + 2] = first_vertex + i + 2;
  }
}

void path::triangulate_convex(Vec<u32> &idx, u32 first_vertex, u32 num_vertices)
{
  if (num_vertices < 3)
  {
    return;
  }

  u32 const num_indices = (num_vertices - 2) * 3;
  u32 const first_index = idx.size32();

  idx.extend_uninit(num_indices).unwrap();

  for (u32 i = 0, v = 1; i < num_indices; i += 3, v++)
  {
    idx[first_index + i]     = first_vertex;
    idx[first_index + i + 1] = first_vertex + v;
    idx[first_index + i + 2] = first_vertex + v + 1;
  }
}

void Canvas::init()
{
}

void Canvas::uninit()
{
  frame_arena.uninit();
  rrect_params.uninit();
  ngon_params.uninit();
  ngon_vertices.uninit();
  ngon_indices.uninit();
  ngon_index_counts.uninit();

  for (Canvas::Pass const &p : passes)
  {
    p.uninit(p.task.data);
  }
  passes.uninit();
}

Canvas &Canvas::reset()
{
  for (Canvas::Pass const &pass : passes)
  {
    pass.uninit(pass.task.data);
  }
  passes.clear();
  rrect_params.clear();
  ngon_params.clear();
  ngon_vertices.clear();
  ngon_indices.clear();
  ngon_index_counts.clear();
  passes.clear();
  batch = {};

  return *this;
}

Canvas &Canvas::begin_recording(Vec2 new_viewport_extent)
{
  reset();

  viewport_extent = new_viewport_extent;
  if (new_viewport_extent.y == 0 || new_viewport_extent.x == 0)
  {
    viewport_aspect_ratio = 1;
  }
  else
  {
    viewport_aspect_ratio = viewport_extent.x / viewport_extent.y;
  }

  // [ ] calculate world to view matrix

  /// @brief generate a model-view-projection matrix for the object in the
  /// canvas space
  /// @param transform object-space transform
  /// @param center canvas-space position/center of the object
  /// @param extent extent of the object (used to scale from the [-1, +1]
  /// object-space coordinate)
  /* constexpr Mat4 mvp(Mat4 const &transform, Vec2 center, Vec2 extent) const
   {
     // [ ] TODO: prevent re-calculating this, it is expensive
     return
         // translate the object to its screen position, using (0, 0) as top
         translate3d(vec3((center / (0.5f * viewport_extent)) - 1, 0)) *
         // scale the object in the -1 to + 1 space
         scale3d(vec3(2 / viewport_extent, 1)) *
         // perform object-space transformation
         transform * scale3d(vec3(extent * 0.5F, 1));
   }*/

  return *this;
}

constexpr RectU clip_to_scissor(gpu::Viewport const &viewport,
                                CRect const &clip, Vec2U surface_extent)
{
  Rect  rect{viewport.offset + clip.center - clip.extent / 2, clip.extent};
  Vec2I offset_i{(i32) rect.offset.x, (i32) rect.offset.y};
  Vec2I extent_i{(i32) rect.extent.x, (i32) rect.extent.y};

  RectU scissor{.offset{(u32) max(0, offset_i.x), (u32) max(0, offset_i.y)},
                .extent{(u32) max(0, extent_i.x), (u32) max(0, extent_i.y)}};

  scissor.offset.x = min(scissor.offset.x, surface_extent.x);
  scissor.offset.y = min(scissor.offset.y, surface_extent.y);
  scissor.extent.x = min(surface_extent.x - scissor.offset.x, scissor.extent.x);
  scissor.extent.y = min(surface_extent.y - scissor.offset.y, scissor.extent.y);

  return scissor;
}

static inline void flush_batch(Canvas &c)
{
  switch (c.batch.type)
  {
    case Canvas::BatchType::RRect:
      c.add_pass(
          "RRect"_span, [batch = c.batch](Canvas::RenderContext const &ctx) {
            RRectPassParams params{.rendering_info = ctx.rt.info,
                                   .scissor        = batch.clip,
                                   .viewport       = ctx.rt.viewport,
                                   .params_ssbo    = ctx.rrects.descriptor,
                                   .textures       = ctx.gpu.texture_views,
                                   .first_instance = batch.objects.offset,
                                   .num_instances  = batch.objects.span};

            ctx.passes.rrect->encode(ctx.gpu, ctx.enc, params);
          });
      c.batch = Canvas::Batch{.type = Canvas::BatchType::None};
      return;

    case Canvas::BatchType::Ngon:
      c.add_pass(
          "Ngon"_span, [batch = c.batch](Canvas::RenderContext const &ctx) {
            NgonPassParams params{
                .rendering_info = ctx.rt.info,
                .scissor        = batch.clip,
                .viewport       = ctx.rt.viewport,
                .vertices_ssbo  = ctx.ngon_vertices.descriptor,
                .indices_ssbo   = ctx.ngon_indices.descriptor,
                .params_ssbo    = ctx.ngons.descriptor,
                .textures       = ctx.gpu.texture_views,
                .index_counts =
                    span(ctx.canvas.ngon_index_counts).slice(batch.objects)};
            ctx.passes.ngon->encode(ctx.gpu, ctx.enc, params);
          });
      c.batch = Canvas::Batch{.type = Canvas::BatchType::None};
      return;

    default:
      return;
  }
}

static inline void add_rrect(Canvas &c, RRectParam const &param,
                             CRect const &clip)
{
  u32 const index = c.rrect_params.size32();
  c.rrect_params.push(param).unwrap();

  if (c.batch.type != Canvas::BatchType::RRect || c.batch.clip != clip)
      [[unlikely]]
  {
    flush_batch(c);
    c.batch = Canvas::Batch{.type = Canvas::BatchType::RRect,
                            .clip = clip,
                            .objects{.offset = index, .span = 1}};
    return;
  }

  c.batch.objects.span++;
}

static inline void add_ngon(Canvas &c, NgonParam const &param,
                            CRect const &clip, u32 num_indices)
{
  u32 const index = c.ngon_params.size32();
  c.ngon_index_counts.push(num_indices).unwrap();
  c.ngon_params.push(param).unwrap();

  if (c.batch.type != Canvas::BatchType::Ngon || c.batch.clip != clip)
      [[unlikely]]
  {
    flush_batch(c);
    c.batch = Canvas::Batch{.type = Canvas::BatchType::Ngon,
                            .clip = clip,
                            .objects{.offset = index, .span = 1}};
    return;
  }

  c.batch.objects.span++;
}

Canvas &Canvas::end_recording()
{
  flush_batch(*this);
  return *this;
}

Canvas &Canvas::clip(CRect const &c)
{
  current_clip = c;
  return *this;
}

constexpr Mat4 object_to_world(Mat4 const &transform, Vec2 center, Vec2 extent)
{
  return transform * translate3d(vec3(center, 0)) *
         scale3d(vec3(extent * 0.5F, 1));
}

Canvas &Canvas::circle(ShapeDesc const &desc)
{
  add_rrect(*this,
            RRectParam{.transform = object_to_world(desc.transform, desc.center,
                                                    desc.extent),
                       .tint      = {desc.tint[0], desc.tint[1], desc.tint[2],
                                     desc.tint[3]},
                       .radii     = {1, 1, 1, 1},
                       .uv        = {desc.uv[0], desc.uv[1]},
                       .tiling    = desc.tiling,
                       .aspect_ratio    = desc.extent.x / desc.extent.y,
                       .stroke          = desc.stroke,
                       .thickness       = desc.thickness / desc.extent.y,
                       .edge_smoothness = desc.edge_smoothness,
                       .sampler         = desc.sampler,
                       .albedo          = desc.texture},
            current_clip);

  return *this;
}

Canvas &Canvas::rect(ShapeDesc const &desc)
{
  add_rrect(*this,
            RRectParam{.transform = object_to_world(desc.transform, desc.center,
                                                    desc.extent),
                       .tint      = {desc.tint[0], desc.tint[1], desc.tint[2],
                                     desc.tint[3]},
                       .radii     = {0, 0, 0, 0},
                       .uv        = {desc.uv[0], desc.uv[1]},
                       .tiling    = desc.tiling,
                       .aspect_ratio    = desc.extent.x / desc.extent.y,
                       .stroke          = desc.stroke,
                       .thickness       = desc.thickness / desc.extent.y,
                       .edge_smoothness = desc.edge_smoothness,
                       .sampler         = desc.sampler,
                       .albedo          = desc.texture},
            current_clip);
  return *this;
}

Canvas &Canvas::rrect(ShapeDesc const &desc)
{
  add_rrect(*this,
            RRectParam{.transform = object_to_world(desc.transform, desc.center,
                                                    desc.extent),
                       .tint      = {desc.tint[0], desc.tint[1], desc.tint[2],
                                     desc.tint[3]},
                       .radii     = desc.corner_radii / desc.extent.y,
                       .uv        = {desc.uv[0], desc.uv[1]},
                       .tiling    = desc.tiling,
                       .aspect_ratio    = desc.extent.x / desc.extent.y,
                       .stroke          = desc.stroke,
                       .thickness       = desc.thickness / desc.extent.y,
                       .edge_smoothness = desc.edge_smoothness,
                       .sampler         = desc.sampler,
                       .albedo          = desc.texture},
            current_clip);
  return *this;
}

Canvas &Canvas::brect(ShapeDesc const &desc)
{
  u32 const first_vertex = ngon_vertices.size32();
  u32 const first_index  = ngon_indices.size32();

  path::brect(ngon_vertices, desc.corner_radii);

  u32 const num_vertices = ngon_vertices.size32() - first_vertex;

  path::triangulate_convex(ngon_indices, first_vertex, num_vertices);

  u32 const num_indices = ngon_indices.size32() - first_index;

  add_ngon(*this,
           NgonParam{
               .transform =
                   object_to_world(desc.transform, desc.center, desc.extent),
               .tint = {desc.tint[0], desc.tint[1], desc.tint[2], desc.tint[3]},
               .uv   = {desc.uv[0], desc.uv[1]},
               .tiling       = desc.tiling,
               .sampler      = desc.sampler,
               .albedo       = desc.texture,
               .first_index  = first_index,
               .first_vertex = first_vertex},
           current_clip, num_indices);

  return *this;
}

Canvas &Canvas::squircle(ShapeDesc const &desc, f32 elasticity, u32 segments)
{
  u32 const first_vertex = ngon_vertices.size32();
  u32 const first_index  = ngon_indices.size32();

  path::squircle(ngon_vertices, elasticity, segments);

  u32 const num_vertices = ngon_vertices.size32() - first_vertex;

  path::triangulate_convex(ngon_indices, first_vertex, num_vertices);

  u32 const num_indices = ngon_indices.size32() - first_index;

  add_ngon(*this,
           NgonParam{
               .transform =
                   object_to_world(desc.transform, desc.center, desc.extent),
               .tint = {desc.tint[0], desc.tint[1], desc.tint[2], desc.tint[3]},
               .uv   = {desc.uv[0], desc.uv[1]},
               .tiling       = desc.tiling,
               .sampler      = desc.sampler,
               .albedo       = desc.texture,
               .first_index  = first_index,
               .first_vertex = first_vertex},
           current_clip, num_indices);

  return *this;
}

Canvas &Canvas::text(ShapeDesc const &desc, TextBlock const &block,
                     TextLayout const &layout, TextBlockStyle const &style,
                     CRect const &clip)
{
  CHECK(style.runs.size() == block.runs.size());
  CHECK(style.runs.size() == block.fonts.size());

  f32 const  block_width = max(layout.extent.x, style.align_width);
  Vec2 const block_extent{block_width, layout.extent.y};

  constexpr u8 PASS_BACKGROUND    = 0;
  constexpr u8 PASS_GLYPH_SHADOWS = 1;
  constexpr u8 PASS_GLYPHS        = 2;
  constexpr u8 PASS_UNDERLINE     = 3;
  constexpr u8 PASS_STRIKETHROUGH = 4;
  constexpr u8 NUM_PASSES         = 5;

  for (u8 pass = 0; pass < NUM_PASSES; pass++)
  {
    f32 line_y = -block_extent.y * 0.5F;
    for (Line const &ln : layout.lines)
    {
      if (!overlaps(clip, CRect{.center = desc.center + line_y,
                                .extent = {block_width, ln.metrics.height}}))
      {
        continue;
      }
      line_y += ln.metrics.height;
      f32 const           baseline  = line_y - ln.metrics.descent;
      TextDirection const direction = level_to_direction(ln.metrics.level);
      // flip the alignment axis direction if it is an RTL line
      f32 const alignment =
          style.alignment *
          ((direction == TextDirection::LeftToRight) ? 1 : -1);
      f32 cursor = space_align(block_width, ln.metrics.width, alignment) -
                   ln.metrics.width * 0.5F;
      for (TextRun const &run :
           span(layout.runs).slice(ln.first_run, ln.num_runs))
      {
        FontStyle const    &font_style = block.fonts[run.style];
        TextStyle const    &run_style  = style.runs[run.style];
        FontInfo const      font       = get_font_info(font_style.font);
        GpuFontAtlas const *atlas      = font.gpu_atlas.value();
        f32 const run_width = au_to_px(run.metrics.advance, run.font_height);

        if (pass == PASS_BACKGROUND && !run_style.background.is_transparent())
        {
          Vec2 extent{run_width, au_to_px(run.metrics.ascent, run.font_height) +
                                     ln.metrics.height};
          Vec2 center =
              Vec2{cursor + extent.x * 0.5F, line_y - extent.y * 0.5F};
          rect({.center    = desc.center,
                .extent    = extent,
                .transform = desc.transform * translate3d(vec3(center, 0)),
                .tint      = run_style.background});
        }

        f32 glyph_cursor = cursor;
        for (u32 g = 0; g < run.num_glyphs; g++)
        {
          GlyphShape const &sh  = layout.glyphs[run.first_glyph + g];
          Glyph const      &gl  = font.glyphs[sh.glyph];
          AtlasGlyph const &agl = atlas->glyphs[sh.glyph];
          Vec2 const extent     = au_to_px(gl.metrics.extent, run.font_height);
          Vec2 const center =
              Vec2{glyph_cursor, baseline} +
              au_to_px(gl.metrics.bearing, run.font_height) * Vec2{1, -1} +
              au_to_px(sh.offset, run.font_height) + extent * 0.5F;

          if (pass == PASS_GLYPH_SHADOWS && run_style.shadow_scale != 0 &&
              !run_style.shadow.is_transparent())
          {
            Vec2 shadow_extent = extent * run_style.shadow_scale;
            Vec2 shadow_center = center + run_style.shadow_offset;
            rect({.center = desc.center,
                  .extent = shadow_extent,
                  .transform =
                      desc.transform * translate3d(vec3(shadow_center, 0)),
                  .tint            = run_style.shadow,
                  .sampler         = desc.sampler,
                  .texture         = atlas->textures[agl.layer],
                  .uv              = {agl.uv[0], agl.uv[1]},
                  .tiling          = desc.tiling,
                  .edge_smoothness = desc.edge_smoothness});
          }

          if (pass == PASS_GLYPHS && !run_style.foreground.is_transparent())
          {
            rect({.center    = desc.center,
                  .extent    = extent,
                  .transform = desc.transform * translate3d(vec3(center, 0)),
                  .tint      = run_style.foreground,
                  .sampler   = desc.sampler,
                  .texture   = atlas->textures[agl.layer],
                  .uv        = {agl.uv[0], agl.uv[1]},
                  .tiling    = desc.tiling,
                  .edge_smoothness = desc.edge_smoothness});
          }

          glyph_cursor += au_to_px(sh.advance.x, run.font_height);
        }

        if (pass == PASS_STRIKETHROUGH &&
            run_style.strikethrough_thickness != 0)
        {
          Vec2 extent{run_width, run_style.strikethrough_thickness};
          Vec2 center =
              Vec2{cursor, baseline - run.font_height * 0.5F} + extent * 0.5F;
          rect({.center    = desc.center,
                .extent    = extent,
                .transform = desc.transform * translate3d(vec3(center, 0)),
                .tint      = run_style.strikethrough,
                .sampler   = desc.sampler,
                .texture   = 0,
                .uv        = {},
                .tiling    = desc.tiling,
                .edge_smoothness = desc.edge_smoothness});
        }

        if (pass == PASS_UNDERLINE && run_style.underline_thickness != 0)
        {
          Vec2 extent{run_width, run_style.underline_thickness};
          Vec2 center = Vec2{cursor, baseline + 2} + extent * 0.5F;
          rect({.center    = desc.center,
                .extent    = extent,
                .transform = desc.transform * translate3d(vec3(center, 0)),
                .tint      = run_style.underline,
                .sampler   = desc.sampler,
                .texture   = 0,
                .uv        = {},
                .tiling    = desc.tiling,
                .edge_smoothness = desc.edge_smoothness});
        }
        cursor += run_width;
      }
    }
  }

  return *this;
}

Canvas &Canvas::triangles(ShapeDesc const &desc, Span<Vec2 const> points)
{
  if (points.size() < 3)
  {
    return *this;
  }

  u32 const first_index  = ngon_indices.size32();
  u32 const first_vertex = ngon_vertices.size32();

  ngon_vertices.extend_copy(points).unwrap();
  path::triangles(first_vertex, points.size32(), ngon_indices);

  u32 const num_indices = ngon_vertices.size32() - first_vertex;

  add_ngon(*this,
           NgonParam{
               .transform =
                   object_to_world(desc.transform, desc.center, desc.extent),
               .tint = {desc.tint[0], desc.tint[1], desc.tint[2], desc.tint[3]},
               .uv   = {desc.uv[0], desc.uv[1]},
               .tiling       = desc.tiling,
               .sampler      = desc.sampler,
               .albedo       = desc.texture,
               .first_index  = first_index,
               .first_vertex = first_vertex},
           current_clip, num_indices);

  return *this;
}

Canvas &Canvas::triangles(ShapeDesc const &desc, Span<Vec2 const> points,
                          Span<u32 const> idx)
{
  if (points.size() < 3)
  {
    return *this;
  }

  u32 const first_index  = ngon_indices.size32();
  u32 const first_vertex = ngon_vertices.size32();

  ngon_vertices.extend_copy(points).unwrap();
  ngon_indices.extend_copy(idx).unwrap();

  for (u32 &v : span(ngon_indices).slice(first_index))
  {
    v += first_vertex;
  }

  add_ngon(*this,
           NgonParam{
               .transform =
                   object_to_world(desc.transform, desc.center, desc.extent),
               .tint = {desc.tint[0], desc.tint[1], desc.tint[2], desc.tint[3]},
               .uv   = {desc.uv[0], desc.uv[1]},
               .tiling       = desc.tiling,
               .sampler      = desc.sampler,
               .albedo       = desc.texture,
               .first_index  = first_index,
               .first_vertex = first_vertex},
           current_clip, idx.size32());

  return *this;
}

Canvas &Canvas::line(ShapeDesc const &desc, Span<Vec2 const> points)
{
  if (points.size() < 2)
  {
    return *this;
  }

  u32 const first_index  = ngon_indices.size32();
  u32 const first_vertex = ngon_vertices.size32();
  path::triangulate_stroke(points, ngon_vertices, ngon_indices,
                           desc.thickness / desc.extent.y);

  u32 const num_indices = ngon_indices.size32() - first_index;

  add_ngon(*this,
           NgonParam{
               .transform =
                   object_to_world(desc.transform, desc.center, desc.extent),
               .tint = {desc.tint[0], desc.tint[1], desc.tint[2], desc.tint[3]},
               .uv   = {desc.uv[0], desc.uv[1]},
               .tiling       = desc.tiling,
               .sampler      = desc.sampler,
               .albedo       = desc.texture,
               .first_index  = first_index,
               .first_vertex = first_vertex},
           current_clip, num_indices);

  return *this;
}

Canvas &Canvas::blur(CRect const &area, u32 num_passes)
{
  flush_batch(*this);

  CHECK(num_passes > 0);
  blur_params.push(CanvasBlurParam{.area = area, .num_passes = num_passes})
      .unwrap();
  add_run(*this, CanvasPassType::Blur);
  return *this;
}

Canvas &Canvas::add_pass(Pass pass)
{
  flush_batch(*this);
  passes.push(pass).unwrap();
  return *this;
}

}        // namespace ash
