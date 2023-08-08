#pragma once

#include "ashura/primitives.h"
#include "ashura/widget.h"

namespace ash
{

struct Padding : public Widget
{
  template <WidgetImpl DerivedWidget>
  Padding(EdgeInsets iedge_insets, DerivedWidget ichild) :
      edge_insets{iedge_insets}
  {
    children.push(new DerivedWidget{std::move(ichild)}).unwrap();
  }

  STX_DISABLE_COPY(Padding)
  STX_DEFAULT_MOVE(Padding)

  virtual ~Padding() override
  {
    for (Widget *child : children)
    {
      delete child;
    }
  }

  template <WidgetImpl DerivedWidget>
  void update_children(DerivedWidget new_child)
  {
    for (Widget *child : children)
    {
      delete child;
    }
    children.clear();
    children.push(new DerivedWidget{std::move(new_child)}).unwrap();
  }

  void update_children(Widget *const new_child)
  {
    for (Widget *child : children)
    {
      delete child;
    }
    children.clear();
    children.push_inplace(new_child).unwrap();
  }

  virtual stx::Span<Widget *const> get_children(Context &ctx) override
  {
    return children;
  }

  virtual WidgetDebugInfo get_debug_info(Context &ctx) override
  {
    return WidgetDebugInfo{.type = "Padding"};
  }

  virtual void allocate_size(Context &ctx, vec2 allocated_size, stx::Span<vec2> children_allocation) override
  {
    children_allocation.fill(max(allocated_size - edge_insets.xy(), vec2{0, 0}));
  }

  virtual vec2 fit(Context &ctx, vec2 allocated_size, stx::Span<vec2 const> children_allocations, stx::Span<vec2 const> children_sizes, stx::Span<vec2> children_positions) override
  {
    children_positions[0] = edge_insets.top_left();
    return min(children_sizes[0] + edge_insets.xy(), allocated_size);
  }

  EdgeInsets         edge_insets;
  stx::Vec<Widget *> children;
};

}        // namespace ash
