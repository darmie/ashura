/// SPDX-License-Identifier: MIT
#pragma once
#include "ashura/engine/color.h"
#include "ashura/engine/font.h"
#include "ashura/engine/passes.h"
#include "ashura/engine/renderer.h"
#include "ashura/engine/scene.h"
#include "ashura/engine/text.h"
#include "ashura/std/allocators.h"
#include "ashura/std/math.h"
#include "ashura/std/types.h"

namespace ash
{

/// @brief all points are stored in the [-1, +1] range, all arguments must be
/// normalized to same range.
namespace path
{
void rect(Vec<Vec2> &vtx);

/// @brief generate vertices for an arc
/// @param segments upper bound on the number of segments to divide the arc
/// into
/// @param start start angle
/// @param stop stop angle
void arc(Vec<Vec2> &vtx, f32 start, f32 stop, u32 segments);

/// @brief generate vertices for a circle
/// @param segments upper bound on the number of segments to divide the circle
/// into
void circle(Vec<Vec2> &vtx, u32 segments);

/// @brief generate vertices for a circle
/// @param segments upper bound on the number of segments to divide the circle
/// into
/// @param degree number of degrees of the super-ellipse
void squircle(Vec<Vec2> &vtx, f32 degree, u32 segments);

/// @brief generate vertices for a circle
/// @param segments upper bound on the number of segments to divide the circle
/// into
/// @param corner_radii border radius of each corner
void rrect(Vec<Vec2> &vtx, Vec4 corner_radii, u32 segments);

/// @brief generate vertices of a bevel rect
/// @param vtx
/// @param slants each component represents the relative distance from the
/// corners of each bevel
void brect(Vec<Vec2> &vtx, Vec4 slants);

/// @brief generate vertices for a quadratic bezier curve
/// @param segments upper bound on the number of segments to divide the bezier
/// curve into
/// @param cp[0-2] control points
void bezier(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, u32 segments);

/// @brief generate vertices for a quadratic bezier curve
/// @param segments upper bound on the number of segments to divide the bezier
/// curve into
/// @param cp[0-3] control points
void cubic_bezier(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, Vec2 cp3,
                  u32 segments);

/// @brief generate a catmull rom spline
/// @param segments upper bound on the number of segments to divide the bezier
/// curve into
/// @param cp[0-3] control points
void catmull_rom(Vec<Vec2> &vtx, Vec2 cp0, Vec2 cp1, Vec2 cp2, Vec2 cp3,
                 u32 segments);

/// @brief triangulate a stroke path, given the vertices for its points
void triangulate_stroke(Span<Vec2 const> points, Vec<Vec2> &vtx, Vec<u32> &idx,
                        f32 thickness);

/// @brief generate indices for a triangle list
void triangles(u32 first_vertex, u32 num_vertices, Vec<u32> &idx);

/// @brief generate vertices for a quadratic bezier curve
void triangulate_convex(Vec<u32> &idx, u32 first_vertex, u32 num_vertices);
};        // namespace path

enum class ScaleMode : u8
{
  Stretch = 0,
  Tile    = 1
};

/// @brief Canvas Shape Description
/// @param center center of the shape in world-space
/// @param extent extent of the shape in world-space
/// @param transform world-space transform matrix
/// @param corner_radii corner radii of each corner if rrect
/// @param stroke lerp intensity between stroke and fill, 0 to fill, 1 to stroke
/// @param thickness thickness of the stroke
/// @param scissor in surface pixel coordinates
/// @param tint Linear Color gradient to use as tint
/// @param sampler sampler to use in rendering the shape
/// @param texture texture index in the global texture store
/// @param uv uv coordinates of the upper-left and lower-right
/// @param tiling tiling factor
/// @param edge_smoothness edge smoothness to apply if it is a rrect
struct ShapeDesc
{
  Vec2 center = {0, 0};

  Vec2 extent = {0, 0};

  Mat4 transform = Mat4::identity();

  Vec4 corner_radii = {0, 0, 0, 0};

  f32 stroke = 0.0f;

  f32 thickness = 1.0f;

  ColorGradient tint = {};

  u32 sampler = 0;

  u32 texture = 0;

  Vec2 uv[2] = {{0, 0}, {1, 1}};

  f32 tiling = 1;

  f32 edge_smoothness = 0.0015F;
};

struct Canvas
{
  struct RenderContext
  {
    Canvas                        &canvas;
    GpuContext                    &gpu;
    PassContext                   &passes;
    RenderTarget const            &rt;
    gpu::CommandEncoderImpl const &enc;
    SSBO const                    &rrects;
    SSBO const                    &ngons;
    SSBO const                    &ngon_vertices;
    SSBO const                    &ngon_indices;
  };

  enum class BatchType : u8
  {
    None  = 0,
    RRect = 1,
    Ngon  = 2
  };

  struct Batch
  {
    BatchType type = BatchType::None;
    CRect     clip{.center{0, 0}, .extent{F32_MAX, F32_MAX}};
    Slice32   objects{};
  };

  struct Pass
  {
    Span<char const>                name{};
    Fn<void(RenderContext const &)> task = fn([](RenderContext const &) {});
    void (*uninit)(void *)               = [](void *) {};
  };

  ArenaPool frame_arena{.min_arena_size = 100_KB, .max_arena_size = 100_KB};

  Vec2 viewport_extent{};

  Vec2U surface_extent{};

  f32 viewport_aspect_ratio = 1;

  Mat4 world_to_view = Mat4::identity();

  CRect current_clip{.center{0, 0}, .extent{F32_MAX, F32_MAX}};

  Vec<RRectParam> rrect_params{};

  Vec<NgonParam> ngon_params{};

  Vec<Vec2> ngon_vertices{};

  Vec<u32> ngon_indices{};

  Vec<u32> ngon_index_counts{};

  Batch batch = {};

  Vec<Pass> passes{};

  void init();

  void uninit();

  Canvas &begin_recording(Vec2 viewport_extent);

  Canvas &end_recording();

  Canvas &reset();

  Canvas &clip(CRect const &area);

  /// @brief render a circle
  Canvas &circle(ShapeDesc const &desc);

  /// @brief render a rectangle
  Canvas &rect(ShapeDesc const &desc);

  /// @brief render a rounded rectangle
  Canvas &rrect(ShapeDesc const &desc);

  /// @brief render a beveled rectangle
  Canvas &brect(ShapeDesc const &desc);

  /// @brief render a squircle (triangulation based)
  /// @param num_segments an upper bound on the number of segments to
  /// @param degree
  Canvas &squircle(ShapeDesc const &desc, f32 degree, u32 segments);

  /// @brief
  ///
  ///
  /// ┏━━━━━━━━━━━━━━━━━┑
  /// ┃  0  ┃  1  ┃  2  ┃
  /// ┃╸╸╸╸╸┃╸╸╸╸╸┃╸╸╸╸╸┃
  /// ┃  3  ┃  4  ┃  5  ┃
  /// ┃╸╸╸╸╸┃╸╸╸╸╸┃╸╸╸╸╸┃
  /// ┃  6  ┃  7  ┃  8  ┃
  /// ┗━━━━━━━━━━━━━━━━━┛
  ///
  /// Scaling:
  ///
  /// 0 2 6 8; None
  /// 1 7; Horizontal
  /// 3 5;	Vertical
  /// 4;	Horizontal + Vertical
  ///
  /// @param desc
  /// @param mode
  /// @param uvs
  Canvas &nine_slice(ShapeDesc const &desc, ScaleMode mode,
                     Span<Vec4 const> uvs);

  /// @brief Render text using font atlases
  /// @param desc only desc.center, desc.transform, desc.tiling, and
  /// desc.sampler are used
  /// @param block Text Block to be rendered
  /// @param layout Layout of text block to be rendered
  /// @param style styling of the text block, contains styling for the runs and
  /// alignment of the block
  /// @param atlases font atlases
  /// @param clip clip rect for culling draw commands of the text block
  Canvas &text(ShapeDesc const &desc, TextBlock const &block,
               TextLayout const &layout, TextBlockStyle const &style,
               CRect const &clip = {{F32_MAX / 2, F32_MAX / 2},
                                    {F32_MAX, F32_MAX}});

  /// @brief Render Non-Indexed Triangles
  Canvas &triangles(ShapeDesc const &desc, Span<Vec2 const> vertices);

  /// @brief Render Indexed Triangles
  Canvas &triangles(ShapeDesc const &desc, Span<Vec2 const> vertices,
                    Span<u32 const> indices);

  /// @brief triangulate and render line
  Canvas &line(ShapeDesc const &desc, Span<Vec2 const> vertices);

  /// @brief perform a Canvas-space blur
  /// @param area region in the canvas to apply the blur to
  /// @param num_passes number of blur passes to execute, higher values result
  /// in blurrier results
  Canvas &blur(CRect const &area, u32 num_passes);

  /// @brief register a custom canvas pass to be executed in the render thread
  Canvas &add_pass(Pass pass);

  template <typename T>
  T *alloc()
  {
    T *out;
    CHECK(frame_arena.nalloc(1, out));
    return out;
  }

  template <typename T>
  Span<T> alloc(usize n)
  {
    T *out;
    CHECK(frame_arena.nalloc(n, out));
    return Span{out, n};
  }

  template <typename Lambda>
  Canvas &add_pass(Span<char const> name, Lambda &&lambda)
  {
    Lambda *lambda = alloc<Lambda>();
    new (lambda) Lambda{(Lambda &&) lambda};
    auto lambda_fn = fn(lamda);

    Pass pass{.name = name, .task = lambda_fn, .uninit = [](void *l) {
                Lambda *lambda = static_cast<Lambda *>(l);
                lambda->~lambda();
              }};

    return add_pass(pass);
  }
};

}        // namespace ash

// we are trying to batch together related render params/commands belonging to
// the same pipeline
//
//