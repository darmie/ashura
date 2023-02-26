#pragma once

#include <chrono>

#include "ashura/canvas.h"
#include "ashura/layout.h"
#include "ashura/widget.h"

namespace ash {

struct WidgetDrawEntry {
  Widget* widget = nullptr;
  Widget* parent = nullptr;
  usize parent_index = 0;
  i64 z_index = 0;
  ash::quad quad;
};

// TODO(lamarrr): more window events pumping to widgets, how?
struct WidgetSystem {
  explicit WidgetSystem(Widget& iroot) : root{&iroot} {}

  static void __assign_ids_recursive(Widget& widget, u64& id) {
    widget.id = id;

    for (Widget* child : widget.get_children()) {
      id++;
      __assign_ids_recursive(*child, id);
    }
  }

  static void __launch_recursive(Widget& widget, WidgetContext& context) {
    widget.on_launch(context);

    for (Widget* child : widget.get_children()) {
      __launch_recursive(*child, context);
    }
  }

  static void __exit_recursive(Widget& widget, WidgetContext& context) {
    widget.on_exit(context);

    for (Widget* child : widget.get_children()) {
      __exit_recursive(*child, context);
    }
  }

  static void __tick_recursive(Widget& widget, WidgetContext& context,
                               std::chrono::nanoseconds interval) {
    widget.tick(context, interval);
    for (Widget* child : widget.get_children()) {
      __tick_recursive(*child, context, interval);
    }
  }

  void __push_recursive(stx::Vec<WidgetDrawEntry>& entries,
                        Widget const* last_hit_widget,
                        bool& last_hit_widget_is_alive, Widget& widget,
                        Widget* parent, i64 z_index) {
    if (widget.get_visibility() == Visibility::Visible) {
      z_index = widget.get_z_index(z_index);

      entries
          .push(WidgetDrawEntry{.widget = &widget,
                                .parent = parent,
                                .z_index = z_index,
                                .quad = quad{}})
          .unwrap();
    }

    if (last_hit_widget == &widget) {
      last_hit_widget_is_alive = true;
    }

    for (Widget* child : widget.get_children()) {
      __push_recursive(entries, last_hit_widget, last_hit_widget_is_alive,
                       *child, &widget, z_index + 1);
    }
  }

  void launch(WidgetContext& context) { __launch_recursive(*root, context); }

  void exit(WidgetContext& context) { __exit_recursive(*root, context); }

  void assign_ids() {
    u64 id = 0;
    __assign_ids_recursive(*root, id);
  }

  void pump_events(WidgetContext& context) {
    for (auto const& e : events) {
      if (std::holds_alternative<MouseClickEvent>(e)) {
        MouseClickEvent event = std::get<MouseClickEvent>(e);

        for (WidgetDrawEntry const* iter = entries.end();
             iter > entries.begin();) {
          iter--;
          if (iter->quad.contains(event.position)) {
            iter->widget->on_click(context, event.button, event.position,
                                   event.clicks, iter->quad);
            break;
          }
        }
      } else if (std::holds_alternative<MouseMotionEvent>(e)) {
        MouseMotionEvent event = std::get<MouseMotionEvent>(e);

        Widget* hit_widget = nullptr;

        for (WidgetDrawEntry const* iter = entries.end();
             iter > entries.begin();) {
          iter--;
          if (iter->quad.contains(event.position)) {
            if (iter->widget != last_hit_widget) {
              iter->widget->on_mouse_enter(context, event.position, iter->quad);
            } else {
              iter->widget->on_mouse_move(context, event.position,
                                          event.translation, iter->quad);
            }
            hit_widget = iter->widget;
            break;
          }
        }

        if (last_hit_widget != nullptr && last_hit_widget_is_alive &&
            last_hit_widget != hit_widget) {
          last_hit_widget->on_mouse_leave(context,
                                          stx::Some(vec2{event.position}));
        }

        last_hit_widget = hit_widget;
        last_hit_widget_is_alive = hit_widget != nullptr;

      } else if (std::holds_alternative<WindowEvents>(e)) {
        if ((std::get<WindowEvents>(e) & WindowEvents::MouseLeave) !=
            WindowEvents::None) {
          if (last_hit_widget != nullptr && last_hit_widget_is_alive) {
            last_hit_widget->on_mouse_leave(context, stx::None);
            last_hit_widget = nullptr;
          }
        }
      }
    }

    events.clear();
  }

  void tick_widgets(WidgetContext& context, std::chrono::nanoseconds interval) {
    __tick_recursive(*root, context, interval);
  }

  void perform_widget_layout(vec2 viewport_extent) {
    ash::perform_layout(*root,
                        rect{.offset = vec2{0, 0}, .extent = viewport_extent});
  }

  void rebuild_draw_entries() {
    entries.clear();
    __push_recursive(entries, last_hit_widget, last_hit_widget_is_alive, *root,
                     nullptr, 0);
    entries.span().sort([](WidgetDrawEntry const& a, WidgetDrawEntry const& b) {
      return a.z_index < b.z_index;
    });
  }

  void draw_widgets(WidgetContext& context, gfx::Canvas& canvas) {
    // what we need to do is to check within the rotated rect
    for (WidgetDrawEntry& entry : entries) {
      canvas.save();
      canvas.transform = entry.widget->get_transform() * canvas.transform;
      if (entry.parent) {
        entry.parent->pre_draw(canvas, *entry.widget, entry.widget->area);
      }
      entry.widget->draw(canvas, entry.widget->area);
      if (entry.parent) {
        entry.parent->post_draw(canvas, *entry.widget, entry.widget->area);
      }
      entry.quad =
          quad{.p1 = transform(canvas.transform, entry.widget->area.offset),
               .p2 = transform(canvas.transform,
                               entry.widget->area.offset +
                                   vec2{entry.widget->area.extent.x, 0}),
               .p3 = transform(canvas.transform, entry.widget->area.offset +
                                                     entry.widget->area.extent),
               .p4 = transform(canvas.transform,
                               entry.widget->area.offset +
                                   vec2{0, entry.widget->area.extent.y})};

      canvas.restore();
    }
  }

  Widget* root = nullptr;
  stx::Vec<std::variant<MouseClickEvent, MouseMotionEvent, WindowEvents>>
      events{stx::os_allocator};
  Widget* last_hit_widget = nullptr;
  bool last_hit_widget_is_alive = false;
  // sorted by z-index
  stx::Vec<WidgetDrawEntry> entries{stx::os_allocator};
};

}  // namespace ash
