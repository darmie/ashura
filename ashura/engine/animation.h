/// SPDX-License-Identifier: MIT
#pragma once
#include "ashura/std/math.h"
#include "ashura/std/types.h"
#include <concepts>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ash
{

/// [ ] https://create.roblox.com/docs/ui/animation#style
/// [ ] Procedural Animation https://www.youtube.com/watch?v=qlfh_rv6khY

// [x] Keyframes, for time interval x, move from a to b
// [ ] keyframe blending
// [x] play
// [x] reverse
// [x] cancel
// [ ] frame-rate customization
// [x] move back from point
// [x] loop n or forever
// [ ] stop after timepoint
// [ ] alternate
// [x] pause
// [x] resume in direction
// [x] reset animation state to beginning

// TODO: Support ash Span type instead of using std::string

// Animatable trait applicable to complex types like Color, Vec2, Vec3 etc
template <typename T>
concept Animatable = requires(T a, T b, f32 t) {
  { lerp(a, b, t) } -> std::convertible_to<T>;
};

namespace animation
{
// Lerp implementations for different types
// Basic arithmetic types
template <typename T>
T lerp(const T &start, const T &end, f32 t)
  requires std::is_arithmetic_v<T>
{
  return ash::lerp<T>(start, end, t);
}
}        // namespace animation

template <Animatable T>
struct Animation;
template <typename T>
struct Keyframe;
template <typename T>
struct Timeline;

struct AnimationTime
{
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration  = std::chrono::duration<float>;

  static f32 get_delta(const TimePoint &current, const TimePoint &previous)
  {
    return std::chrono::duration_cast<Duration>(current - previous).count();
  }
};

enum class CurveType
{
  Linear,
  EaseIn,
  EaseOut,
  EaseInOut,
  Custom
};

struct AnimationConfig
{
  f32         duration = 1.0f;
  f32         delay    = 0.0f;
  bool        loop     = false;
  CurveType   easing   = CurveType::Linear;
  std::string label    = "default";
};

struct AnimationState
{
  bool is_playing   = false;
  f32  current_time = 0.0f;
  f32  delay        = 0.0f;
};

struct TimelineOptions
{
  f32         delay    = 0.0f;
  bool        autoplay = true;
  bool        loop     = false;
  std::string label    = "default";
};

enum class GridDirection
{
  RowMajor,          // Left to right, top to bottom
  ColumnMajor        // Top to bottom, left to right
};

struct StaggerOptions
{
  f32 start = 0.0f;
  f32 from  = 0.0f;
  struct Grid
  {
    i32           rows      = 1;
    i32           cols      = 1;
    GridDirection direction = GridDirection::RowMajor;
  } grid;
  std::function<f32(i32 row, i32 col, i32 index, i32 total)> easing = nullptr;
};

static std::pair<i32, i32> grid_position(i32 index, i32 rows, i32 cols,
                                         GridDirection direction)
{
  if (direction == GridDirection::RowMajor)
  {
    return {
        index / cols,        // row
        index % cols         // col
    };
  }
  else
  {
    return {
        index % rows,        // row
        index / rows         // col
    };
  }
}

static f32 stagger_grid_delay(i32 index, i32 count,
                              const StaggerOptions &stagger_opts)
{
  auto [row, col] =
      grid_position(index, stagger_opts.grid.rows, stagger_opts.grid.cols,
                    stagger_opts.grid.direction);

  if (!stagger_opts.easing)
  {
    return stagger_opts.easing(row, col, index, count);
  }
  else
  {
    // Default grid-based delay calculation
    f32 normalized_row = static_cast<f32>(row) / (stagger_opts.grid.rows - 1);
    f32 normalized_col = static_cast<f32>(col) / (stagger_opts.grid.cols - 1);

    switch (stagger_opts.grid.direction)
    {
      case GridDirection::RowMajor:
        return normalized_row + (normalized_col * 0.5f);
      case GridDirection::ColumnMajor:
        return normalized_col + (normalized_row * 0.5f);
    }
  }
}

static f32
    stagger_linear_delay(i32 i, i32 count,
                         const StaggerOptions &stagger_opts = StaggerOptions{})
{
  return stagger_opts.easing ? stagger_opts.easing(0, 0, i, count) :
                               static_cast<f32>(i) / count;
}

static f32 stagger_delay(i32 i, i32 count,
                         const StaggerOptions &stagger_opts = StaggerOptions{})
{
  bool is_grid_based = stagger_opts.grid.rows > 1 || stagger_opts.grid.cols > 1;

  if (is_grid_based)
  {
    return stagger_grid_delay(i, count, stagger_opts);
  }

  return stagger_linear_delay(i, count, stagger_opts);
}

enum class SegmentType
{
  Basic,            // Single Animation
  Keyframed,        // Multiple Keyframes
  Parallel,         // Multiple parallel animations
  Staggered         // Staggered animations
};

template <Animatable T>
struct TimelineSegment
{
  std::string label    = "default";
  f32         start    = 0.0f;
  f32         duration = 0.0f;
  SegmentType type     = SegmentType::Basic;

  virtual ~TimelineSegment()                      = default;
  virtual void        update(f32 time, f32 delta) = 0;
  virtual void        reset()                     = 0;
  virtual std::string get_label()
  {
    return label;
  }
  virtual T value() const = 0;

private:
  T interpolated_value;
};

template <Animatable T>
struct BasicSegment : TimelineSegment<T>
{
  Animation<T> animation;

  BasicSegment(const T &start, const T &end, AnimationConfig config) :
      animation(start, end, config, this->onUpdate)
  {
    this->duration = config.duration;
    this->type     = SegmentType::Basic;
  }
  void update(f32 current_time, f32 delta) override;
  void reset() override;
  T    value() const override;
};

template <Animatable T>
struct KeyframeSegment : TimelineSegment<T>
{
  std::vector<Keyframe<T>> keyframes;
  f32                      current_time = 0.0f;

  KeyframeSegment(const std::vector<Keyframe<T>> &kf) : keyframes(kf)
  {
    this->type = SegmentType::Keyframed;
    calculate_duration();
  }

  void calculate_duration();

  void update(f32 _time, f32 delta);

  void reset();
  T    value() const override;
};

struct IAnimation
{
  virtual ~IAnimation()           = default;
  virtual void tick()             = 0;
  virtual void play()             = 0;
  virtual void pause()            = 0;
  virtual void resume()           = 0;
  virtual void reset()            = 0;
  virtual bool is_playing() const = 0;
};

// Simple linear animation: perform basic interpolation on opacity, color,
// rotation etc.
// - play
// - pause
// - reset
// - set a callback function to track animation update
// ----
//
// ## Translate Position
// ```
// auto position = std::make_shared<Vec2>();
// Animation anim = Animation<f32>(
//  0.0f, // start
//  10.0f, // end
//  AnimationConfig{.duration = 2.0f, .loop = true, .easing =
//  CurveType::EaseInOut},
//);
// anim.play();
//
// // tick
// position->x = anim.tick(); // update only x axis
//
// ```
//
template <Animatable T>
struct Animation : IAnimation
{
private:
  T    current_value;
  bool is_reset;

public:
  T                        start;
  T                        end;
  AnimationConfig          config;
  AnimationState           state;
  AnimationTime::TimePoint last_update_time;

  Animation(T start, T end, AnimationConfig cfg) :
      start(start), end(end), config(cfg)
  {
    state.delay      = config.delay;
    is_reset         = false;
    last_update_time = AnimationTime::Clock::now();
  }

  void tick() override
  {
    // std::cout << std::format();
    if (!state.is_playing)
      return;

    auto current_time = AnimationTime::Clock::now();
    f32  delta       = AnimationTime::get_delta(current_time, last_update_time);
    last_update_time = current_time;

    run(delta);
  }

  void play() override;

  void pause() override;

  void resume() override;

  void reset() override;

  bool is_playing() const override;

  inline const T value()
  {
    return current_value;
  }

private:
  void run(f32 delta);
  void update(f32 _time, f32 delta);
};

// Animation utility to create and manage or more animatins.
// - play animations
// - pause animations
// - reset animations
// - staggered animations
// - set a callback function to track animation updates
// ```
// AnimationManager animator;
//
// auto anim1 = animator.create<Vec2>(
//     start_pos,
//     end_pos,
//     AnimationConfig{.duration = 1.0f, .loop = true, .easing =
//     CurveType::EaseInOut},
// );
//
// auto anim2 = animator.create<Vec2>(
//     start_pos,
//     end_pos,
//     AnimationConfig{.duration = 1.0f, .loop = true, .easing =
//     CurveType::EaseInOut},
// );
//
// // Play individual animation
// animator.get_animation(0).play();
// animator.get_animation(1).play();
// // Or play all the animations
// animator.play_all();
//
// // Get value from animation
// anim1_value = animator.get_animation(0).value();
// anim2_value = animator.get_animation(1).value();
// ```
//
//
// ### Staggered (Grid) animations
//--------
// ### Example: Ripple Pattern Animation
//```
// animator.stagger<f32>(
//  0.0f, // start
//  1.0f, // end
//  AnimationConfig{.duration = 1.0f, .loop = true},
//  10, // total count
//  StaggerOptions {
//      .grid = {4, 4},
//      .easing = [](int row, int col, int index, int total) {
//          // Ripple effect
//          f32 center_row = 1.5f;
//          f32 center_col = 1.5f;
//          f32 distance = std::sqrt(
//              std::pow(row - center_row, 2) +
//              std::pow(col - center_col, 2)
//          );
//          return distance * 0.15f;
//  },
//  [](const f32& value, index) { /* update logic */ }
//);
//
// animator.play_all();
//```
struct AnimationManager
{
private:
  std::vector<std::shared_ptr<IAnimation>> animations;

public:
  void reserve(u32 size);

  void clear();

  template <typename T>
  std::shared_ptr<Animation<T>> create(T start, T end, AnimationConfig config)
  {
    auto anim = std::make_shared<Animation<T>>(start, end, config);
    animations.push_back(anim);
    return anim;
  }

  void tick();

  void play_all();

  void pause_all();

  void reset_all();

  template <typename T>
  void stagger(const T &start, const T &end, const AnimationConfig &base_config,
               usize               count,
               const StaggerOptions &stagger_opts = StaggerOptions{},
               std::function<void(const T &, usize)> callback = nullptr);

  template <Animatable T>
  std::shared_ptr<Animation<T>> get_animation(usize index)
  {
    if (index >= animations.size())
      return nullptr;
    return std::dynamic_pointer_cast<Animation<T>>(animations[index]);
  }
};

struct KeyframeBase
{
  f32                     duration;
  std::string             key;
  CurveType               curve_type;
  std::function<f32(f32)> custom_easing;

  KeyframeBase(f32 dur, std::string k, CurveType curve = CurveType::Linear) :
      duration(dur), key(std::move(k)), curve_type(curve)
  {
  }

  KeyframeBase(f32 dur, std::string k, std::function<f32(f32)> easing) :
      duration(dur),
      key(std::move(k)),
      curve_type(CurveType::Custom),
      custom_easing(std::move(easing))
  {
  }

  f32 get_ease_fn(f32 t) const
  {
    if (curve_type == CurveType::Custom && custom_easing)
    {
      return custom_easing(t);
    }

    switch (curve_type)
    {
      case CurveType::Linear:
        return linear(t);
      case CurveType::EaseIn:
        return ease_in(t);
      case CurveType::EaseOut:
        return ease_out(t);
      case CurveType::EaseInOut:
        return ease_in_out(t);
      default:
        return t;
    }
  }
};

template <typename T>
struct Keyframe : KeyframeBase
{
  T value;

  Keyframe(f32 dur, std::string k, T val, CurveType curve = CurveType::Linear) :
      KeyframeBase(dur, std::move(k), curve), value(std::move(val))
  {
  }

  Keyframe(f32 dur, std::string k, T val, std::function<f32(f32)> easing) :
      KeyframeBase(dur, std::move(k), std::move(easing)), value(std::move(val))
  {
  }
};

// Timeline: A more robust way to manage keyframe or multiple animations
// -  Timeline animations or keyframes can be run in parallel or staggered.
// -  Set a callback to track each animation or keyframe update.
// -  Direction control
// -  Playback control
// ```
// TimelineOptions options{
//         .autoplay = true,
//         .loop = true
// };
// Timeline<Vec2> timeline(options);
//
// std::vector<Keyframe<Vec2>> keyframes = {
//     Keyframe<Vec2>(0.5f, "start", {0, 100.0f}, CurveType::EaseInOut),
//     Keyframe<Vec2>(0.5f, "middle", {150.0f, 50.0f}, CurveType::EaseInOut),
//     Keyframe<Vec2>(0.5f, "end", {0, 200.0f}, CurveType::EaseInOut)
// };
//
// timeline.add(keyframes);
// // Basic controls
// timeline.play();
// timeline.pause();
// timeline.reverse();
// timeline.seek(2.0f);
// timeline.seek_percentage(0.5f);
//
//  // Playback controls
// timeline.play_from_start();  // Forward from start
// timeline.play_from_end();    // Reverse from end
// timeline.stop();          // Reset to start
// ```
// ---
//
// ### Chainable calls
// ```
// timeline
//     .add(start, end, config)                  // Simple animation
//     .add(keyframes)                           // Keyframe sequence
//     .parallel(configs, values)                // Parallel animations
//     .stagger(start, end, config, count);      // Staggered animations
// ```
template <typename T>
struct Timeline
{
  using SegmentPtr = std::shared_ptr<TimelineSegment<T>>;

  std::vector<SegmentPtr>  segments;
  TimelineOptions          options;
  f32                      current_time   = 0.0f;
  bool                     is_playing     = false;
  f32                      total_duration = 0.0f;
  f32                      time_direction = 1.0f;
  AnimationTime::TimePoint last_update_time;

  Timeline(const TimelineOptions &opts = TimelineOptions{}) : options(opts)
  {
    last_update_time = AnimationTime::Clock::now();
    if (options.autoplay)
    {
      is_playing = true;
    }
  }

  Timeline &reserve(u32 size);

  void clear();

  Timeline &add(const T &start, const T &end, AnimationConfig config);

  Timeline &add(const std::vector<Keyframe<T>> &keyframes,
                std::optional<std::string>      label);

  Timeline &parallel(const std::vector<AnimationConfig> &configs,
                     const std::vector<std::pair<T, T>> &values);

  Timeline &stagger(const T &start, const T &end, AnimationConfig config,
                    size_t                count,
                    const StaggerOptions &stagger_opts = StaggerOptions{});

  void update();

  void play();

  void play_from_start();

  void play_from_end();

  void pause();
  void resume();

  void stop();

  void seek(f32 time);

  // Seek to percentage of duration
  void seek_percentage(f32 percentage);
  void reverse();
  // Force a direction: `1` is forward or `-1` is backward
  void set_direction(i32 direction);

  f32  get_current_time() const;
  f32  get_duration() const;
  f32  get_progress() const;
  bool is_reversed() const;
  bool is_finished() const;

  SegmentPtr get_segment(std::string label);
  SegmentPtr get_segment(u32 index);

private:
  void update_segments(f32 delta);
};

}        // namespace ash
