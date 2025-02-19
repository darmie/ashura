/// SPDX-License-Identifier: MIT
#include "ashura/engine/window.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include "ashura/gpu/vulkan.h"
#include "ashura/std/error.h"
#include "ashura/std/sparse_vec.h"
#include "ashura/std/vec.h"

namespace ash
{

#define CHECKSdl(cond_expr) CHECK_DESC(cond_expr, "SDL Error: ", SDL_GetError())

namespace sdl
{
struct WindowEventListener
{
  Fn<void(WindowEvent const &)> callback = fn([](WindowEvent const &) {});
  WindowEventTypes              types    = WindowEventTypes::None;
};

struct WindowImpl
{
  SDL_Window                         *win       = nullptr;
  gpu::Surface                        surface   = nullptr;
  SDL_WindowID                        id        = 0;
  SparseVec<Vec<WindowEventListener>> listeners = {};
  gpu::InstanceImpl                   instance  = {};
  Fn<WindowRegion(Vec2U)>             hit_test =
      fn([](Vec2U) { return WindowRegion::Normal; });
};

struct SystemEventListener
{
  Fn<void(SystemEvent const &)> callback = fn([](SystemEvent const &) {});
  SystemEventTypes              types    = SystemEventTypes::None;
};

struct ClipBoardImpl : ClipBoard
{
  virtual Result<Void, Void> get(Span<char const> mime, Vec<u8> &out) override
  {
    char mime_c_str[256];
    CHECK(mem::to_c_str(mime, span(mime_c_str)));
    usize mime_data_len;
    void *data = SDL_GetClipboardData(mime_c_str, &mime_data_len);
    if (data == nullptr)
    {
      return Err{};
    }
    defer data_{[&] { SDL_free(data); }};

    out.extend_copy(Span<u8 const>{(u8 *) data, mime_data_len}).unwrap();
    return Ok{};
  }

  virtual Result<Void, Void> set(Span<char const> mime,
                                 Span<u8 const>   data) override
  {
    if (data.is_empty() || mime.is_empty())
    {
      if (SDL_ClearClipboardData())
      {
        return Ok{};
      }
      return Err{};
    }

    char mime_c_str[256];
    CHECK(mem::to_c_str(mime, span(mime_c_str)));

    char const *mime_types[] = {mime_c_str};

    Vec<u8> *data_ctx;
    CHECK(default_allocator.nalloc(1, data_ctx));
    new (data_ctx) Vec<u8>{};

    data_ctx->extend_copy(data).unwrap();

    bool failed = SDL_SetClipboardData(
        [](void *userdata, const char *mime_type, usize *size) -> void const * {
          if (mime_type == nullptr)
          {
            *size = 0;
            return nullptr;
          }
          Vec<u8> *ctx = (Vec<u8> *) userdata;
          *size        = ctx->size();
          return ctx->data();
        },
        [](void *userdata) {
          Vec<u8> *ctx = (Vec<u8> *) userdata;
          ctx->uninit();
          default_allocator.ndealloc(ctx, 1);
        },
        data_ctx, mime_types, 1);

    if (failed)
    {
      return Err{};
    }

    return Ok{};
  }
};

struct WindowSystemImpl : WindowSystem
{
  SparseVec<Vec<SystemEventListener>> listeners;
  ClipBoardImpl                       clipboard;

  SDL_Window *hnd(Window window)
  {
    return ((WindowImpl *) window)->win;
  }

  virtual void init() override
  {
    CHECKSdl(SDL_Init(SDL_INIT_VIDEO));
  }

  virtual void uninit() override
  {
    listeners.uninit();
    SDL_Quit();
  }

  Option<Window> create_window(gpu::InstanceImpl instance,
                               Span<char const>  title) override
  {
    char *title_c_str;
    if (!default_allocator.nalloc(title.size() + 1, title_c_str))
    {
      return None;
    }

    defer title_c_str_{
        [&] { default_allocator.ndealloc(title_c_str, title.size() + 1); }};

    mem::copy(title, title_c_str);
    title_c_str[title.size()] = 0;

    SDL_Window *window = SDL_CreateWindow(
        title_c_str, 1920, 1080, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    CHECKSdl(window != nullptr);
    SDL_WindowID id = SDL_GetWindowID(window);
    CHECKSdl(id != 0);

    CHECK(instance.interface->get_backend(instance.self) ==
          gpu::Backend::Vulkan);

    vk::Instance *vk_instance = (vk::Instance *) instance.self;
    VkSurfaceKHR  surface;

    CHECKSdl(SDL_Vulkan_CreateSurface(window, vk_instance->vk_instance, nullptr,
                                      &surface));

    WindowImpl *impl;

    CHECK(default_allocator.nalloc(1, impl));

    new (impl) WindowImpl{.win      = window,
                          .surface  = (gpu::Surface) surface,
                          .id       = id,
                          .instance = instance};

    SDL_PropertiesID props_id = SDL_GetWindowProperties(window);
    CHECK(SDL_SetPointerProperty(props_id, "impl", impl));

    return Some{(Window) impl};
  }

  void uninit_window(Window window) override
  {
    if (window != nullptr)
    {
      WindowImpl *win = (WindowImpl *) window;
      win->instance->uninit_surface(win->instance.self, win->surface);
      SDL_DestroyWindow(win->win);
      win->listeners.reset();
      default_allocator.ndealloc(win, 1);
    }
  }

  void set_title(Window window, Span<char const> title) override
  {
    char *title_c_str;
    CHECK(default_allocator.nalloc(title.size() + 1, title_c_str));

    defer title_c_str_{
        [&] { default_allocator.ndealloc(title_c_str, title.size() + 1); }};

    mem::copy(title, title_c_str);
    title_c_str[title.size()] = 0;

    CHECKSdl(SDL_SetWindowTitle(hnd(window), title_c_str));
  }

  char const *get_title(Window window) override
  {
    char const *title = SDL_GetWindowTitle(hnd(window));
    CHECKSdl(title != nullptr);
    return title;
  }

  void maximize(Window window) override
  {
    CHECKSdl(SDL_MaximizeWindow(hnd(window)));
  }

  void minimize(Window window) override
  {
    CHECKSdl(SDL_MinimizeWindow(hnd(window)));
  }

  void set_size(Window window, Vec2U size) override
  {
    CHECKSdl(SDL_SetWindowSize(hnd(window), static_cast<int>(size.x),
                               static_cast<int>(size.y)));
  }

  void center(Window window) override
  {
    CHECKSdl(SDL_SetWindowPosition(hnd(window), SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED));
  }

  Vec2U get_size(Window window) override
  {
    int width, height;
    CHECKSdl(SDL_GetWindowSize(hnd(window), &width, &height));
    return Vec2U{static_cast<u32>(width), static_cast<u32>(height)};
  }

  Vec2U get_surface_size(Window window) override
  {
    int width, height;
    CHECKSdl(SDL_GetWindowSizeInPixels(hnd(window), &width, &height));
    return Vec2U{static_cast<u32>(width), static_cast<u32>(height)};
  }

  void set_position(Window window, Vec2I pos) override
  {
    CHECKSdl(SDL_SetWindowPosition(hnd(window), pos.x, pos.y));
  }

  Vec2I get_position(Window window) override
  {
    int x, y;
    CHECKSdl(SDL_GetWindowPosition(hnd(window), &x, &y));
    return Vec2I{x, y};
  }

  void set_min_size(Window window, Vec2U min) override
  {
    CHECKSdl(SDL_SetWindowMinimumSize(hnd(window), static_cast<int>(min.x),
                                      static_cast<int>(min.y)));
  }

  Vec2U get_min_size(Window window) override
  {
    int width, height;
    CHECKSdl(SDL_GetWindowMinimumSize(hnd(window), &width, &height));
    return Vec2U{static_cast<u32>(width), static_cast<u32>(height)};
  }

  void set_max_size(Window window, Vec2U max) override
  {
    CHECKSdl(SDL_SetWindowMaximumSize(hnd(window), static_cast<int>(max.x),
                                      static_cast<int>(max.y)));
  }

  Vec2U get_max_size(Window window) override
  {
    int width, height;
    CHECKSdl(SDL_GetWindowMaximumSize(hnd(window), &width, &height));
    return Vec2U{static_cast<u32>(width), static_cast<u32>(height)};
  }

  void set_icon(Window window, ImageSpan<u8 const, 4> image,
                gpu::Format format) override
  {
    SDL_PixelFormat fmt = SDL_PIXELFORMAT_RGBA8888;

    switch (format)
    {
      case gpu::Format::R8G8B8A8_UNORM:
        fmt = SDL_PIXELFORMAT_RGBA8888;
        break;
      case gpu::Format::B8G8R8A8_UNORM:
        fmt = SDL_PIXELFORMAT_BGRA8888;
        break;
      default:
        CHECK_DESC(false, "unsupported image format");
    }

    SDL_Surface *icon = SDL_CreateSurfaceFrom(
        static_cast<int>(image.width), static_cast<int>(image.height), fmt,
        (void *) image.channels.data(), static_cast<int>(image.pitch()));
    CHECKSdl(icon != nullptr);
    CHECKSdl(SDL_SetWindowIcon(hnd(window), icon));
    SDL_DestroySurface(icon);
  }

  void make_bordered(Window window) override
  {
    CHECKSdl(SDL_SetWindowBordered(hnd(window), true));
  }

  void make_borderless(Window window) override
  {
    CHECKSdl(SDL_SetWindowBordered(hnd(window), false));
  }

  void show(Window window) override
  {
    CHECKSdl(SDL_ShowWindow(hnd(window)));
  }

  void hide(Window window) override
  {
    CHECKSdl(SDL_HideWindow(hnd(window)));
  }

  void raise(Window window) override
  {
    CHECKSdl(SDL_RaiseWindow(hnd(window)));
  }

  void restore(Window window) override
  {
    CHECKSdl(SDL_RestoreWindow(hnd(window)));
  }

  void request_attention(Window window, bool briefly) override
  {
    CHECKSdl(SDL_FlashWindow(hnd(window), briefly ? SDL_FLASH_BRIEFLY :
                                                    SDL_FLASH_UNTIL_FOCUSED));
  }

  void make_fullscreen(Window window) override
  {
    CHECKSdl(SDL_SetWindowFullscreen(hnd(window), true));
  }

  void make_windowed(Window window) override
  {
    CHECKSdl(SDL_SetWindowFullscreen(hnd(window), false));
  }

  void make_resizable(Window window) override
  {
    CHECKSdl(SDL_SetWindowResizable(hnd(window), true));
  }

  void make_unresizable(Window window) override
  {
    CHECKSdl(SDL_SetWindowResizable(hnd(window), false));
  }

  u64 listen(SystemEventTypes              event_types,
             Fn<void(SystemEvent const &)> callback) override
  {
    return listeners
        .push(SystemEventListener{.callback = callback, .types = event_types})
        .unwrap();
  }

  u64 listen(Window window, WindowEventTypes event_types,
             Fn<void(WindowEvent const &)> callback) override
  {
    WindowImpl *pwin = (WindowImpl *) window;
    return pwin->listeners.push(WindowEventListener{callback, event_types})
        .unwrap();
  }

  void unlisten(Window window, u64 listener) override
  {
    WindowImpl *pwin = (WindowImpl *) window;
    pwin->listeners.erase(listener);
  }

  static SDL_HitTestResult sdl_hit_test(SDL_Window *, SDL_Point const *area,
                                        void *data)
  {
    WindowImpl  *win    = (WindowImpl *) data;
    WindowRegion region = win->hit_test(Vec2U{(u32) area->x, (u32) area->y});
    switch (region)
    {
      case WindowRegion::Normal:
        return SDL_HITTEST_NORMAL;
      case WindowRegion::Draggable:
        return SDL_HITTEST_DRAGGABLE;
      case WindowRegion::ResizeTopLeft:
        return SDL_HITTEST_RESIZE_TOPLEFT;
      case WindowRegion::ResizeTop:
        return SDL_HITTEST_RESIZE_TOP;
      case WindowRegion::ResizeTopRight:
        return SDL_HITTEST_RESIZE_TOPRIGHT;
      case WindowRegion::ResizeRight:
        return SDL_HITTEST_RESIZE_RIGHT;
      case WindowRegion::ResizeBottomRight:
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
      case WindowRegion::ResizeBottom:
        return SDL_HITTEST_RESIZE_BOTTOM;
      case WindowRegion::ResizeBottomLeft:
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;
      case WindowRegion::ResizeLeft:
        return SDL_HITTEST_RESIZE_LEFT;
      default:
        return SDL_HITTEST_NORMAL;
    }
  }

  Result<Void, Void> set_hit_test(Window                  window,
                                  Fn<WindowRegion(Vec2U)> hit) override
  {
    WindowImpl *pwin = (WindowImpl *) window;
    pwin->hit_test   = hit;
    if (SDL_SetWindowHitTest(pwin->win, sdl_hit_test, pwin) != 0)
    {
      return Err{};
    }

    return Ok{};
  }

  gpu::Surface get_surface(Window window) override
  {
    WindowImpl *pwin = (WindowImpl *) window;
    return pwin->surface;
  }

  void push_window_event(SDL_WindowID window_id, WindowEvent const &event)
  {
    SDL_Window *win = SDL_GetWindowFromID(window_id);
    CHECK(win != nullptr);
    SDL_PropertiesID props_id = SDL_GetWindowProperties(win);
    WindowImpl      *impl =
        (WindowImpl *) SDL_GetPointerProperty(props_id, "impl", nullptr);
    CHECK(impl != nullptr);

    for (WindowEventListener const &listener : impl->listeners.dense.v0)
    {
      if (has_bits(listener.types, event.type))
      {
        listener.callback(event);
      }
    }
  }

  void push_system_event(SystemEvent const &event)
  {
    for (SystemEventListener const &listener : listeners.dense.v0)
    {
      if (has_bits(listener.types, event.type))
      {
        listener.callback(event);
      }
    }
  }

  void poll_events() override
  {
    SDL_Event event;

    while (SDL_PollEvent(&event) == true)
    {
      switch (event.type)
      {
        case SDL_EVENT_WINDOW_SHOWN:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Shown});
          return;
        case SDL_EVENT_WINDOW_HIDDEN:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Hidden});
          return;
        case SDL_EVENT_WINDOW_EXPOSED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Exposed});
          return;
        case SDL_EVENT_WINDOW_MOVED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Moved});
          return;
        case SDL_EVENT_WINDOW_RESIZED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Resized});
          return;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0,
                          .type  = WindowEventTypes::SurfaceResized});
          return;
        case SDL_EVENT_WINDOW_MINIMIZED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Minimized});
          return;
        case SDL_EVENT_WINDOW_MAXIMIZED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Maximized});
          return;
        case SDL_EVENT_WINDOW_RESTORED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Restored});
          return;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::MouseEnter});
          return;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::MouseLeave});
          return;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::FocusIn});
          return;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::FocusOut});
          return;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0,
                          .type  = WindowEventTypes::CloseRequested});
          return;
        case SDL_EVENT_WINDOW_OCCLUDED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Occluded});
          break;
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0,
                          .type  = WindowEventTypes::EnterFullScreen});
          break;
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0,
                          .type  = WindowEventTypes::LeaveFullScreen});
          break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
          MouseClickEvent mouse_event{.id = event.button.which,
                                      .position =
                                          Vec2{event.button.x, event.button.y},
                                      .clicks = event.button.clicks};
          switch (event.button.button)
          {
            case SDL_BUTTON_LEFT:
              mouse_event.button = MouseButtons::Primary;
              break;
            case SDL_BUTTON_RIGHT:
              mouse_event.button = MouseButtons::Secondary;
              break;
            case SDL_BUTTON_MIDDLE:
              mouse_event.button = MouseButtons::Middle;
              break;
            case SDL_BUTTON_X1:
              mouse_event.button = MouseButtons::A1;
              break;
            case SDL_BUTTON_X2:
              mouse_event.button = MouseButtons::A2;
              break;
            default:
              UNREACHABLE();
          }

          switch (event.type)
          {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
              mouse_event.action = KeyAction::Press;
              break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
              mouse_event.action = KeyAction::Release;
              break;

            default:
              return;
          }

          push_window_event(event.button.windowID,
                            WindowEvent{.mouse_click = mouse_event,
                                        .type = WindowEventTypes::MouseClick});
          return;
        }

        case SDL_EVENT_MOUSE_MOTION:
          push_window_event(
              event.motion.windowID,
              WindowEvent{
                  .mouse_motion =
                      MouseMotionEvent{
                          .id       = event.motion.which,
                          .position = Vec2{event.motion.x, event.motion.y},
                          .translation =
                              Vec2{event.motion.xrel, event.motion.yrel}},
                  .type = WindowEventTypes::MouseMotion});
          return;

        case SDL_EVENT_MOUSE_WHEEL:
          push_window_event(
              event.wheel.windowID,
              WindowEvent{
                  .mouse_wheel =
                      MouseWheelEvent{
                          .id = event.wheel.which,
                          .position =
                              Vec2{event.wheel.mouse_x, event.wheel.mouse_y},
                          .translation = Vec2{event.wheel.x, event.wheel.y}},
                  .type = WindowEventTypes::MouseWheel});
          return;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
          push_window_event(
              event.key.windowID,
              WindowEvent{
                  .key  = KeyEvent{.scan_code = (ScanCode) (event.key.scancode),
                                   .key_code  = (KeyCode) ((u16) event.key.key &
                                                         ~SDLK_SCANCODE_MASK),
                                   .modifiers =
                                      static_cast<KeyModifiers>(event.key.mod),
                                   .action = event.type == SDL_EVENT_KEY_DOWN ?
                                                 KeyAction::Press :
                                                 KeyAction::Release},
                  .type = WindowEventTypes::Key});
          return;

        case SDL_EVENT_WINDOW_DESTROYED:
          push_window_event(
              event.window.windowID,
              WindowEvent{.none_ = 0, .type = WindowEventTypes::Destroyed});
          return;

        case SDL_EVENT_TEXT_EDITING:
        case SDL_EVENT_TEXT_INPUT:
          // [ ] handle
          return;

        case SDL_EVENT_DROP_BEGIN:
          // [ ] check
          return;

        case SDL_EVENT_DROP_POSITION:
          //  [ ] check
          logger->info("drop pos:", "x: ", event.drop.x, ", y", event.drop.y);
          return;

        case SDL_EVENT_DROP_FILE:
          // [ ] how to handle in view system
          logger->info("x: ", event.drop.x, "y: ", event.drop.y,
                       "text: ", event.drop.data);
          return;

        case SDL_EVENT_DROP_TEXT:
          logger->info("x: ", event.drop.x, "y: ", event.drop.y,
                       "text: ", event.drop.data);
          return;

        case SDL_EVENT_SYSTEM_THEME_CHANGED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::ThemeChanged});
          return;

        case SDL_EVENT_KEYMAP_CHANGED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::KeymapChanged});
          return;

        case SDL_EVENT_AUDIO_DEVICE_ADDED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::AudioDeviceAdded});
          return;

        case SDL_EVENT_AUDIO_DEVICE_REMOVED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::AudioDeviceRemoved});
          return;

        case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::AudioDeviceFormatChanged});
          return;

        case SDL_EVENT_DISPLAY_ORIENTATION:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::DisplayReoriented});
          return;

        case SDL_EVENT_DISPLAY_ADDED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::DisplayAdded});
          return;

        case SDL_EVENT_DISPLAY_REMOVED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::DisplayRemoved});
          return;

        case SDL_EVENT_DISPLAY_MOVED:
          push_system_event(
              SystemEvent{.type = SystemEventTypes::DisplayMoved});
          return;

        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
          return;

        default:
          return;
      }
    }
  }

  virtual ClipBoard &get_clipboard() override
  {
    return clipboard;
  }
};

}        // namespace sdl

static sdl::WindowSystemImpl sdl_window_system_impl;

WindowSystem *sdl_window_system = &sdl_window_system_impl;

}        // namespace ash