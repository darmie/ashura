#include "ashura/std/format.h"
#include <charconv>
#include <string.h>

namespace ash
{

template <typename IntT>
bool push_int(fmt::Context &ctx, fmt::Spec const &spec, IntT value)
{
  int base = 0;

  switch (spec.style)
  {
    case fmt::Style::Decimal:
    {
      base = 10;
    }
    break;
    case fmt::Style::Octal:
    {
      base = 8;
    }
    break;
    case fmt::Style::Hex:
    {
      base = 16;
    }
    break;
    case fmt::Style::Bin:
    {
      base = 1;
    }
    break;
    default:
    {
      base = 10;
    }
    break;
  }

  std::to_chars_result result = std::to_chars(
      ctx.scratch_buffer.begin(), ctx.scratch_buffer.end(), value, base);
  if (result.ec == std::errc{})
  {
    return ctx.push(Span{ctx.scratch_buffer.begin(),
                         (usize) (result.ptr - ctx.scratch_buffer.begin())});
  }

  return false;
}

template <typename FloatT>
bool push_float(fmt::Context &ctx, fmt::Spec const &spec, FloatT value)
{
  std::chars_format format = std::chars_format::general;
  switch (spec.style)
  {
    case fmt::Style::General:
      format = std::chars_format::general;
      break;
    case fmt::Style::Scientific:
      format = std::chars_format::scientific;
      break;
    default:
      format = std::chars_format::general;
      break;
  }

  std::to_chars_result result{};
  if (spec.precision > 0)
  {
    result = std::to_chars(ctx.scratch_buffer.begin(), ctx.scratch_buffer.end(),
                           value, format, spec.precision);
  }
  else
  {
    result = std::to_chars(ctx.scratch_buffer.begin(), ctx.scratch_buffer.end(),
                           value, format);
  }

  if (result.ec == std::errc{})
  {
    return ctx.push(Span{ctx.scratch_buffer.begin(),
                         (usize) (result.ptr - ctx.scratch_buffer.begin())});
  }
  return false;
}

bool fmt::push_object_hex(fmt::Context &ctx, fmt::Spec const &spec,
                          char const *name, Span<u8 const> rep)
{
  Spec hex_spec{.style = Style::Hex, .precision = spec.precision};
  bool ok =
      push(ctx, spec, "Hex:") && push(ctx, spec, name) && push(ctx, spec, "[");
  u8 const *it = rep.data();
  while (ok && it < rep.end())
  {
    ok = ok && push(ctx, spec, "0x") && push(ctx, hex_spec, *it) &&
         ((it != rep.end() - 1) ? push(ctx, spec, ", ") : true);
    it++;
  }

  ok = ok && push(ctx, spec, "]");
  return ok;
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, bool value)
{
  return ctx.push(value ? "true"_span : "false"_span);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, u8 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, u16 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, u32 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, u64 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, i8 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, i16 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, i32 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, i64 value)
{
  return push_int(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, f32 value)
{
  return push_float(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, f64 value)
{
  return push_float(ctx, spec, value);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec2 const &value)
{
  return push(ctx, spec, "Vec2{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec3 const &value)
{
  return push(ctx, spec, "Vec3{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec4 const &value)
{
  return push(ctx, spec, "Vec4{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.w) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec2I const &value)
{
  return push(ctx, spec, "Vec2I{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec3I const &value)
{
  return push(ctx, spec, "Vec3I{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec4I const &value)
{
  return push(ctx, spec, "Vec4I{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.w) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec2U const &value)
{
  return push(ctx, spec, "Vec2I{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec3U const &value)
{
  return push(ctx, spec, "Vec3U{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &spec, Vec4U const &value)
{
  return push(ctx, spec, "Vec4U{") && push(ctx, spec, value.x) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.y) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.z) &&
         push(ctx, spec, ", ") && push(ctx, spec, value.w) &&
         push(ctx, spec, "}");
}

bool fmt::push(fmt::Context &, fmt::Spec &spec, fmt::Spec const &value)
{
  spec = value;
  return true;
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, Span<char const> str)
{
  return ctx.push(str);
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, std::string_view str)
{
  return ctx.push(to_span(str));
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, std::string const &str)
{
  return ctx.push(to_span(str));
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, char const *str)
{
  return ctx.push(Span{str, strlen(str)});
}

bool fmt::push(fmt::Context &ctx, fmt::Spec const &, void const *ptr)
{
  Spec const ptr_spec{.style = Style::Hex};
  return ctx.push("0x"_span) && push_int(ctx, ptr_spec, (uintptr_t) ptr);
}

}        // namespace ash