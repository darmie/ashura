/// SPDX-License-Identifier: MIT
#pragma once
#include "ashura/std/cfg.h"
#include "ashura/std/traits.h"
#include <bit>
#include <cfloat>
#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <new>
#include <type_traits>

namespace ash
{
typedef char8_t   c8;
typedef char16_t  c16;
typedef char32_t  c32;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef float     f32;
typedef double    f64;
typedef size_t    usize;
typedef ptrdiff_t isize;
typedef uintptr_t uptr;
typedef intptr_t  iptr;
typedef u64       Hash;

constexpr u8 U8_MIN = 0;
constexpr u8 U8_MAX = 0xFF;

constexpr i8 I8_MIN = -0x7F - 1;
constexpr i8 I8_MAX = 0x7F;

constexpr u16 U16_MIN = 0;
constexpr u16 U16_MAX = 0xFFFF;

constexpr i16 I16_MIN = -0x7FFF - 1;
constexpr i16 I16_MAX = 0x7FFF;

constexpr u32 U32_MIN = 0;
constexpr u32 U32_MAX = 0xFFFFFFFFU;

constexpr i32 I32_MIN = -0x7FFFFFFF - 1;
constexpr i32 I32_MAX = 0x7FFFFFFF;

constexpr u64 U64_MIN = 0;
constexpr u64 U64_MAX = 0xFFFFFFFFFFFFFFFFULL;

constexpr i64 I64_MIN = -0x7FFFFFFFFFFFFFFFLL - 1;
constexpr i64 I64_MAX = 0x7FFFFFFFFFFFFFFFLL;

constexpr usize USIZE_MIN = 0;
constexpr usize USIZE_MAX = SIZE_MAX;

constexpr isize ISIZE_MIN = PTRDIFF_MIN;
constexpr isize ISIZE_MAX = PTRDIFF_MAX;

constexpr f32 F32_MIN          = -FLT_MAX;
constexpr f32 F32_MIN_POSITIVE = FLT_MIN;
constexpr f32 F32_MAX          = FLT_MAX;
constexpr f32 F32_EPSILON      = FLT_EPSILON;
constexpr f32 F32_INFINITY     = INFINITY;

constexpr f64 F64_MIN          = -DBL_MAX;
constexpr f64 F64_MIN_POSITIVE = DBL_MIN;
constexpr f64 F64_MAX          = DBL_MAX;
constexpr f64 F64_EPSILON      = DBL_EPSILON;
constexpr f64 F64_INFINITY     = INFINITY;

constexpr f32 PI = 3.14159265358979323846F;

struct Noop
{
  constexpr void operator()(auto &&...) const
  {
  }
};

struct Add
{
  constexpr auto operator()(auto const &a, auto const &b) const
  {
    return a + b;
  }
};

struct Sub
{
  constexpr auto operator()(auto const &a, auto const &b) const
  {
    return a - b;
  }
};

struct Mul
{
  constexpr auto operator()(auto const &a, auto const &b) const
  {
    return a * b;
  }
};

struct Div
{
  constexpr auto operator()(auto const &a, auto const &b) const
  {
    return a / b;
  }
};

struct Equal
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a == b;
  }
};

struct NotEqual
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a != b;
  }
};

struct Lesser
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a < b;
  }
};

struct LesserOrEqual
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a <= b;
  }
};

struct Greater
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a > b;
  }
};

struct GreaterOrEqual
{
  constexpr bool operator()(auto const &a, auto const &b) const
  {
    return a >= b;
  }
};

struct Compare
{
  constexpr int operator()(auto const &a, auto const &b) const
  {
    if (a == b)
    {
      return 0;
    }
    if (a > b)
    {
      return -1;
    }
    return 1;
  }
};

struct Min
{
  template <typename T>
  constexpr T const &operator()(T const &a, T const &b) const
  {
    return a < b ? a : b;
  }
};

struct Max
{
  template <typename T>
  constexpr auto const &operator()(T const &a, T const &b) const
  {
    return a > b ? a : b;
  }
};

struct Swap
{
  template <typename T>
  constexpr void operator()(T &a, T &b) const
  {
    T a_tmp{(T &&) a};
    a = (T &&) b;
    b = (T &&) a_tmp;
  }
};

struct Clamp
{
  template <typename T>
  constexpr T const &operator()(T const &value, T const &min,
                                T const &max) const
  {
    return value < min ? min : (value > max ? max : value);
  }
};

constexpr Noop           noop;
constexpr Add            add;
constexpr Sub            sub;
constexpr Mul            mul;
constexpr Div            div;
constexpr Equal          equal;
constexpr NotEqual       not_equal;
constexpr Lesser         lesser;
constexpr LesserOrEqual  lesser_or_equal;
constexpr Greater        greater;
constexpr GreaterOrEqual greater_or_equal;
constexpr Compare        compare;
constexpr Min            min;
constexpr Max            max;
constexpr Swap           swap;
constexpr Clamp          clamp;

template <typename T>
constexpr bool has_bits(T src, T cmp)
{
  return (src & cmp) == cmp;
}

template <typename T>
constexpr bool has_any_bit(T src, T cmp)
{
  return (src & cmp) != (T) 0;
}

constexpr bool is_pow2(u8 x)
{
  return (x & (x - 1)) == 0U;
}

constexpr bool is_pow2(u16 x)
{
  return (x & (x - 1)) == 0U;
}

constexpr bool is_pow2(u32 x)
{
  return (x & (x - 1)) == 0U;
}

constexpr bool is_pow2(u64 x)
{
  return (x & (x - 1)) == 0ULL;
}

constexpr bool get_bit(u8 s, usize i)
{
  return (s >> i) & 1;
}

constexpr bool get_bit(u16 s, usize i)
{
  return (s >> i) & 1;
}

constexpr bool get_bit(u32 s, usize i)
{
  return (s >> i) & 1;
}

constexpr bool get_bit(u64 s, usize i)
{
  return (s >> i) & 1;
}

constexpr void clear_bit(u8 &s, usize i)
{
  s &= ~(((u8) 1) << i);
}

constexpr void clear_bit(u16 &s, usize i)
{
  s &= ~(((u16) 1) << i);
}

constexpr void clear_bit(u32 &s, usize i)
{
  s &= ~(((u32) 1) << i);
}

constexpr void clear_bit(u64 &s, usize i)
{
  s &= ~(((u64) 1) << i);
}

constexpr void set_bit(u8 &s, usize i)
{
  s |= (((u8) 1) << i);
}

constexpr void set_bit(u16 &s, usize i)
{
  s |= (((u16) 1) << i);
}

constexpr void set_bit(u32 &s, usize i)
{
  s |= (((u32) 1) << i);
}

constexpr void set_bit(u64 &s, usize i)
{
  s |= (((u64) 1) << i);
}

constexpr void assign_bit(u8 &s, usize i, bool b)
{
  s &= ~(((u8) 1) << i);
  s |= (((u8) b) << i);
}

constexpr void assign_bit(u16 &s, usize i, bool b)
{
  s &= ~(((u16) 1) << i);
  s |= (((u16) b) << i);
}

constexpr void assign_bit(u32 &s, usize i, bool b)
{
  s &= ~(((u32) 1) << i);
  s |= (((u32) b) << i);
}

constexpr void assign_bit(u64 &s, usize i, bool b)
{
  s &= ~(((u64) 1) << i);
  s |= (((u64) b) << i);
}

constexpr void flip_bit(u8 &s, usize i)
{
  s = s ^ (((usize) 1) << i);
}

constexpr void flip_bit(u16 &s, usize i)
{
  s = s ^ (((usize) 1) << i);
}

constexpr void flip_bit(u32 &s, usize i)
{
  s = s ^ (((usize) 1) << i);
}

constexpr void flip_bit(u64 &s, usize i)
{
  s = s ^ (((usize) 1) << i);
}

constexpr unsigned long long operator""_KB(unsigned long long x)
{
  return x << 10;
}

constexpr unsigned long long operator""_MB(unsigned long long x)
{
  return x << 20;
}

constexpr unsigned long long operator""_GB(unsigned long long x)
{
  return x << 30;
}

constexpr unsigned long long operator""_TB(unsigned long long x)
{
  return x << 40;
}

template <typename T>
struct NumTraits;

template <>
struct NumTraits<u8>
{
  static constexpr u8   NUM_BITS       = 8;
  static constexpr u8   LOG2_NUM_BITS  = 3;
  static constexpr u8   MIN            = U8_MIN;
  static constexpr u8   MAX            = U8_MAX;
  static constexpr bool SIGNED         = false;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<u16>
{
  static constexpr u8   NUM_BITS       = 16;
  static constexpr u8   LOG2_NUM_BITS  = 4;
  static constexpr u16  MIN            = U16_MIN;
  static constexpr u16  MAX            = U16_MAX;
  static constexpr bool SIGNED         = false;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<u32>
{
  static constexpr u8   NUM_BITS       = 32;
  static constexpr u8   LOG2_NUM_BITS  = 5;
  static constexpr u32  MIN            = U32_MIN;
  static constexpr u32  MAX            = U32_MAX;
  static constexpr bool SIGNED         = false;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<u64>
{
  static constexpr u8   NUM_BITS       = 64;
  static constexpr u8   LOG2_NUM_BITS  = 6;
  static constexpr u64  MIN            = U64_MIN;
  static constexpr u64  MAX            = U64_MAX;
  static constexpr bool SIGNED         = false;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<i8>
{
  static constexpr u8   NUM_BITS       = 8;
  static constexpr u8   LOG2_NUM_BITS  = 3;
  static constexpr i8   MIN            = I8_MIN;
  static constexpr i8   MAX            = I8_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<i16>
{
  static constexpr u8   NUM_BITS       = 16;
  static constexpr u8   LOG2_NUM_BITS  = 4;
  static constexpr i16  MIN            = I16_MIN;
  static constexpr i16  MAX            = I16_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<i32>
{
  static constexpr u8   NUM_BITS       = 32;
  static constexpr u8   LOG2_NUM_BITS  = 5;
  static constexpr i32  MIN            = I32_MIN;
  static constexpr i32  MAX            = I32_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<i64>
{
  static constexpr u8   NUM_BITS       = 64;
  static constexpr u8   LOG2_NUM_BITS  = 6;
  static constexpr i64  MIN            = I64_MIN;
  static constexpr i64  MAX            = I64_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = false;
};

template <>
struct NumTraits<f32>
{
  static constexpr u8   NUM_BITS       = 32;
  static constexpr u8   LOG2_NUM_BITS  = 5;
  static constexpr f32  MIN            = F32_MIN;
  static constexpr f32  MAX            = F32_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = true;
};

template <>
struct NumTraits<f64>
{
  static constexpr u8   NUM_BITS       = 64;
  static constexpr u8   LOG2_NUM_BITS  = 6;
  static constexpr f64  MIN            = F64_MIN;
  static constexpr f64  MAX            = F64_MAX;
  static constexpr bool SIGNED         = true;
  static constexpr bool FLOATING_POINT = true;
};

template <typename T>
struct NumTraits<T const> : NumTraits<T>
{
};

template <typename T>
struct NumTraits<T volatile> : NumTraits<T>
{
};

template <typename T>
struct NumTraits<T const volatile> : NumTraits<T>
{
};

template <typename Repr, usize NumBits>
inline constexpr usize BIT_PACKS =
    (NumBits + (NumTraits<Repr>::NUM_BITS - 1)) >>
    NumTraits<Repr>::LOG2_NUM_BITS;

template <typename Repr>
constexpr usize bit_packs(usize num_bits)
{
  return (num_bits + (NumTraits<Repr>::NUM_BITS - 1)) >>
         NumTraits<Repr>::LOG2_NUM_BITS;
}

/// regular void
struct Void
{
};

template <typename E>
using enum_ut = std::underlying_type_t<E>;

template <typename E>
constexpr enum_ut<E> enum_uv(E a)
{
  return static_cast<enum_ut<E>>(a);
}

template <typename E>
constexpr enum_ut<E> enum_uv_or(E a, E b)
{
  return static_cast<enum_ut<E>>(enum_uv(a) | enum_uv(b));
}

template <typename E>
constexpr enum_ut<E> enum_uv_and(E a, E b)
{
  return static_cast<enum_ut<E>>(enum_uv(a) & enum_uv(b));
}

template <typename E>
constexpr enum_ut<E> enum_uv_not(E a)
{
  return static_cast<enum_ut<E>>(~enum_uv(a));
}

template <typename E>
constexpr enum_ut<E> enum_uv_xor(E a, E b)
{
  return static_cast<enum_ut<E>>(enum_uv(a) ^ enum_uv(b));
}

template <typename E>
constexpr E enum_or(E a, E b)
{
  return static_cast<E>(enum_uv_or(a, b));
}

template <typename E>
constexpr E enum_and(E a, E b)
{
  return static_cast<E>(enum_uv_and(a, b));
}

template <typename E>
constexpr E enum_xor(E a, E b)
{
  return static_cast<E>(enum_uv_xor(a, b));
}

template <typename E>
constexpr E enum_not(E a)
{
  return static_cast<E>(enum_uv_not(a));
}

#define ASH_DEFINE_ENUM_BIT_OPS(E)   \
  constexpr E operator|(E a, E b)    \
  {                                  \
    return ::ash::enum_or(a, b);     \
  }                                  \
                                     \
  constexpr E operator&(E a, E b)    \
  {                                  \
    return ::ash::enum_and(a, b);    \
  }                                  \
                                     \
  constexpr E operator^(E a, E b)    \
  {                                  \
    return ::ash::enum_xor(a, b);    \
  }                                  \
                                     \
  constexpr E operator~(E a)         \
  {                                  \
    return ::ash::enum_not(a);       \
  }                                  \
                                     \
  constexpr E &operator|=(E &a, E b) \
  {                                  \
    a = a | b;                       \
    return a;                        \
  }                                  \
                                     \
  constexpr E &operator&=(E &a, E b) \
  {                                  \
    a = a & b;                       \
    return a;                        \
  }                                  \
                                     \
  constexpr E &operator^=(E &a, E b) \
  {                                  \
    a = a ^ b;                       \
    return a;                        \
  }

template <typename... T>
struct Tuple
{
  static constexpr u8 NUM_ELEMENTS = 0;
};

Tuple() -> Tuple<>;

template <typename T0>
struct Tuple<T0>
{
  typedef T0 Type0;

  static constexpr u8 NUM_ELEMENTS = 1;

  T0 v0{};
};

template <typename T0>
Tuple(T0) -> Tuple<T0>;

template <typename T0, typename T1>
struct Tuple<T0, T1>
{
  typedef T0 Type0;
  typedef T1 Type1;

  static constexpr u8 NUM_ELEMENTS = 2;

  T0 v0{};
  T1 v1{};
};

template <typename T0, typename T1>
Tuple(T0, T1) -> Tuple<T0, T1>;

template <typename T0, typename T1, typename T2>
struct Tuple<T0, T1, T2>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;

  static constexpr u8 NUM_ELEMENTS = 3;

  T0 v0{};
  T1 v1{};
  T2 v2{};
};

template <typename T0, typename T1, typename T2>
Tuple(T0, T1, T2) -> Tuple<T0, T1, T2>;

template <typename T0, typename T1, typename T2, typename T3>
struct Tuple<T0, T1, T2, T3>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;

  static constexpr u8 NUM_ELEMENTS = 4;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
};

template <typename T0, typename T1, typename T2, typename T3>
Tuple(T0, T1, T2, T3) -> Tuple<T0, T1, T2, T3>;

template <typename T0, typename T1, typename T2, typename T3, typename T4>
struct Tuple<T0, T1, T2, T3, T4>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;

  static constexpr u8 NUM_ELEMENTS = 5;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4>
Tuple(T0, T1, T2, T3, T4) -> Tuple<T0, T1, T2, T3, T4>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
struct Tuple<T0, T1, T2, T3, T4, T5>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;
  typedef T5 Type5;

  static constexpr u8 NUM_ELEMENTS = 6;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
  T5 v5{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
Tuple(T0, T1, T2, T3, T4, T5) -> Tuple<T0, T1, T2, T3, T4, T5>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6>
struct Tuple<T0, T1, T2, T3, T4, T5, T6>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;
  typedef T5 Type5;
  typedef T6 Type6;

  static constexpr u8 NUM_ELEMENTS = 7;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
  T5 v5{};
  T6 v6{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6>
Tuple(T0, T1, T2, T3, T4, T5, T6) -> Tuple<T0, T1, T2, T3, T4, T5, T6>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;
  typedef T5 Type5;
  typedef T6 Type6;
  typedef T7 Type7;

  static constexpr u8 NUM_ELEMENTS = 8;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
  T5 v5{};
  T6 v6{};
  T7 v7{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7) -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;
  typedef T5 Type5;
  typedef T6 Type6;
  typedef T7 Type7;
  typedef T8 Type8;

  static constexpr u8 NUM_ELEMENTS = 9;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
  T5 v5{};
  T6 v6{};
  T7 v7{};
  T8 v8{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>
{
  typedef T0 Type0;
  typedef T1 Type1;
  typedef T2 Type2;
  typedef T3 Type3;
  typedef T4 Type4;
  typedef T5 Type5;
  typedef T6 Type6;
  typedef T7 Type7;
  typedef T8 Type8;
  typedef T9 Type9;

  static constexpr u8 NUM_ELEMENTS = 10;

  T0 v0{};
  T1 v1{};
  T2 v2{};
  T3 v3{};
  T4 v4{};
  T5 v5{};
  T6 v6{};
  T7 v7{};
  T8 v8{};
  T9 v9{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;

  static constexpr u8 NUM_ELEMENTS = 11;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;
  typedef T11 Type11;

  static constexpr u8 NUM_ELEMENTS = 12;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
  T11 v11{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;
  typedef T11 Type11;
  typedef T12 Type12;

  static constexpr u8 NUM_ELEMENTS = 13;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
  T11 v11{};
  T12 v12{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;
  typedef T11 Type11;
  typedef T12 Type12;
  typedef T13 Type13;

  static constexpr u8 NUM_ELEMENTS = 14;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
  T11 v11{};
  T12 v12{};
  T13 v13{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13, typename T14>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;
  typedef T11 Type11;
  typedef T12 Type12;
  typedef T13 Type13;
  typedef T14 Type14;

  static constexpr u8 NUM_ELEMENTS = 15;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
  T11 v11{};
  T12 v12{};
  T13 v13{};
  T14 v14{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13, typename T14>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>;

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13, typename T14,
          typename T15>
struct Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14,
             T15>
{
  typedef T0  Type0;
  typedef T1  Type1;
  typedef T2  Type2;
  typedef T3  Type3;
  typedef T4  Type4;
  typedef T5  Type5;
  typedef T6  Type6;
  typedef T7  Type7;
  typedef T8  Type8;
  typedef T9  Type9;
  typedef T10 Type10;
  typedef T11 Type11;
  typedef T12 Type12;
  typedef T13 Type13;
  typedef T14 Type14;
  typedef T15 Type15;

  static constexpr u8 NUM_ELEMENTS = 16;

  T0  v0{};
  T1  v1{};
  T2  v2{};
  T3  v3{};
  T4  v4{};
  T5  v5{};
  T6  v6{};
  T7  v7{};
  T8  v8{};
  T9  v9{};
  T10 v10{};
  T11 v11{};
  T12 v12{};
  T13 v13{};
  T14 v14{};
  T15 v15{};
};

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9,
          typename T10, typename T11, typename T12, typename T13, typename T14,
          typename T15>
Tuple(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)
    -> Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14,
             T15>;

template <unsigned int Index, typename Tuple>
constexpr auto &&impl_get(Tuple &&tuple)
{
  static_assert(Index < remove_ref<Tuple>::NUM_ELEMENTS,
                "Index of Tuple Elements out of bounds");
  if constexpr (Index == 0)
  {
    return tuple.v0;
  }
  if constexpr (Index == 1)
  {
    return tuple.v1;
  }
  if constexpr (Index == 2)
  {
    return tuple.v2;
  }
  if constexpr (Index == 3)
  {
    return tuple.v3;
  }
  if constexpr (Index == 4)
  {
    return tuple.v4;
  }
  if constexpr (Index == 5)
  {
    return tuple.v5;
  }
  if constexpr (Index == 6)
  {
    return tuple.v6;
  }
  if constexpr (Index == 7)
  {
    return tuple.v7;
  }
  if constexpr (Index == 8)
  {
    return tuple.v8;
  }
  if constexpr (Index == 9)
  {
    return tuple.v9;
  }
  if constexpr (Index == 10)
  {
    return tuple.v10;
  }
  if constexpr (Index == 11)
  {
    return tuple.v11;
  }
  if constexpr (Index == 12)
  {
    return tuple.v12;
  }
  if constexpr (Index == 13)
  {
    return tuple.v13;
  }
  if constexpr (Index == 14)
  {
    return tuple.v14;
  }
  if constexpr (Index == 15)
  {
    return tuple.v15;
  }
}

template <u8 Index, typename... T>
constexpr auto const &get(Tuple<T...> const &tuple)
{
  return impl_get<Index>(tuple);
}

template <u8 Index, typename... T>
constexpr auto &get(Tuple<T...> &tuple)
{
  return impl_get<Index>(tuple);
}

template <u8 Index, typename... T>
constexpr auto const &&get(Tuple<T...> const &&tuple)
{
  return impl_get<Index>(tuple);
}

template <u8 Index, typename... T>
constexpr auto &&get(Tuple<T...> &&tuple)
{
  return impl_get<Index>(tuple);
}

template <typename Fn, typename Tuple>
constexpr void impl_for_each(Fn &&op, Tuple &&tuple)
{
  constexpr u8 NUM_ELEMENTS = remove_ref<Tuple>::NUM_ELEMENTS;
  if constexpr (NUM_ELEMENTS > 0)
  {
    op(tuple.v0);
  }
  if constexpr (NUM_ELEMENTS > 1)
  {
    op(tuple.v1);
  }
  if constexpr (NUM_ELEMENTS > 2)
  {
    op(tuple.v2);
  }
  if constexpr (NUM_ELEMENTS > 3)
  {
    op(tuple.v3);
  }
  if constexpr (NUM_ELEMENTS > 4)
  {
    op(tuple.v4);
  }
  if constexpr (NUM_ELEMENTS > 5)
  {
    op(tuple.v5);
  }
  if constexpr (NUM_ELEMENTS > 6)
  {
    op(tuple.v6);
  }
  if constexpr (NUM_ELEMENTS > 7)
  {
    op(tuple.v7);
  }
  if constexpr (NUM_ELEMENTS > 8)
  {
    op(tuple.v8);
  }
  if constexpr (NUM_ELEMENTS > 9)
  {
    op(tuple.v9);
  }
  if constexpr (NUM_ELEMENTS > 10)
  {
    op(tuple.v10);
  }
  if constexpr (NUM_ELEMENTS > 11)
  {
    op(tuple.v11);
  }
  if constexpr (NUM_ELEMENTS > 12)
  {
    op(tuple.v12);
  }
  if constexpr (NUM_ELEMENTS > 13)
  {
    op(tuple.v13);
  }
  if constexpr (NUM_ELEMENTS > 14)
  {
    op(tuple.v14);
  }
  if constexpr (NUM_ELEMENTS > 15)
  {
    op(tuple.v15);
  }
}

template <typename Fn, typename... T>
constexpr void for_each(Fn &&op, Tuple<T...> const &tuple)
{
  impl_for_each((Fn &&) op, tuple);
}

template <typename Fn, typename... T>
constexpr void for_each(Fn &&op, Tuple<T...> &tuple)
{
  impl_for_each((Fn &&) op, tuple);
}

template <typename Fn, typename... T>
constexpr void for_each(Fn &&op, Tuple<T...> &&tuple)
{
  impl_for_each((Fn &&) op, tuple);
}

struct Slice
{
  usize offset = 0;
  usize span   = 0;

  constexpr usize end() const
  {
    return offset + span;
  }

  constexpr Slice operator()(usize size) const
  {
    // written such that overflow will not occur even if both offset and span
    // are set to USIZE_MAX
    usize o = offset > size ? size : offset;
    usize s = ((size - o) > span) ? span : size - o;
    return Slice{o, s};
  }

  constexpr bool is_empty() const
  {
    return span == 0;
  }
};

struct Slice32
{
  u32 offset = 0;
  u32 span   = 0;

  constexpr usize end() const
  {
    return offset + span;
  }

  constexpr Slice32 operator()(u32 size) const
  {
    // written such that overflow will not occur even if both offset and span
    // are set to U32_MAX
    u32 o = offset > size ? size : offset;
    u32 s = ((size - o) > span) ? span : size - o;
    return Slice32{o, s};
  }

  constexpr bool is_empty() const
  {
    return span == 0;
  }

  constexpr operator Slice() const
  {
    return Slice{offset, span};
  }
};

struct Slice64
{
  u64 offset = 0;
  u64 span   = 0;

  constexpr u64 end() const
  {
    return offset + span;
  }

  constexpr Slice64 operator()(u64 size) const
  {
    // written such that overflow will not occur even if both offset and span
    // are set to U64_MAX
    u64 o = offset > size ? size : offset;
    u64 s = ((size - o) > span) ? span : size - o;
    return Slice64{o, s};
  }

  constexpr bool is_empty() const
  {
    return span == 0;
  }

  constexpr operator Slice() const
  {
    return Slice{offset, span};
  }
};

template <typename T, usize N>
constexpr T *begin(T (&a)[N])
{
  return a;
}

template <typename T>
constexpr auto begin(T &&a) -> decltype(a.begin())
{
  return a.begin();
}

template <typename T, usize N>
constexpr T *end(T (&a)[N])
{
  return a + N;
}

template <typename T>
constexpr auto end(T &&a) -> decltype(a.end())
{
  return a.end();
}

template <typename T, usize N>
constexpr T *data(T (&a)[N])
{
  return a;
}

template <typename T>
constexpr auto data(T &&a) -> decltype(a.data())
{
  return a.data();
}

template <typename T, usize N>
constexpr usize size(T (&)[N])
{
  return N;
}

template <typename T>
constexpr auto size(T &&a) -> decltype(a.size())
{
  return a.size();
}

template <typename T, usize N>
constexpr usize size_bytes(T (&)[N])
{
  return sizeof(T) * N;
}

template <typename T>
constexpr auto size_bytes(T &&a) -> decltype(a.size())
{
  return sizeof(T) * a.size();
}

template <typename T, usize N>
constexpr usize size_bits(T (&)[N])
{
  return sizeof(T) * 8 * N;
}

template <typename T>
constexpr auto size_bits(T &&a) -> decltype(a.size())
{
  return sizeof(T) * 8 * a.size();
}

template <typename T>
constexpr auto is_empty(T &&a)
{
  return size(a) == 0;
}

template <typename It>
concept InputIterator = requires(It it) {
  {
    // deref
    *it
  };
  {
    // advance
    it = it++
  };
  {
    // advance
    it = ++it
  };
  {
    // distance
    it = it + (it - it)
  };
  {
    // equal
    (it == it) && true
  };
  {
    // not equal
    (it != it) && true
  };
};

template <typename It>
concept OutputIterator = InputIterator<It> && requires(It it) {
  {
    // assign
    *it = *it
  };
};

template <typename R>
concept InputRange = requires(R r) {
  { begin(r) } -> InputIterator;
  { end(r) } -> InputIterator;
};

template <typename R>
concept OutputRange = requires(R r) {
  { begin(r) } -> OutputIterator;
  { end(r) } -> OutputIterator;
};

template <typename T, usize N>
struct Array
{
  using Type                  = T;
  static constexpr usize SIZE = N;

  T data_[SIZE]{};

  constexpr bool is_empty() const
  {
    return false;
  }

  constexpr T *data()
  {
    return data_;
  }

  constexpr T const *data() const
  {
    return data_;
  }

  constexpr usize size() const
  {
    return SIZE;
  }

  constexpr usize size_bytes() const
  {
    return sizeof(T) * SIZE;
  }

  constexpr T const *begin() const
  {
    return data_;
  }

  constexpr T *begin()
  {
    return data_;
  }

  constexpr T const *end() const
  {
    return data_ + SIZE;
  }

  constexpr T *end()
  {
    return data_ + SIZE;
  }

  constexpr T &operator[](usize index)
  {
    return data_[index];
  }

  constexpr T const &operator[](usize index) const
  {
    return data_[index];
  }

  constexpr operator T const *() const
  {
    return data_;
  }

  constexpr operator T *()
  {
    return data_;
  }
};

template <typename Repr, usize N>
using Bits = Repr[BIT_PACKS<Repr, N>];

template <typename Repr, usize N>
using BitArray = Array<Repr, BIT_PACKS<Repr, N>>;

template <typename T>
struct Span
{
  using Type = T;
  using Repr = T;

  T    *data_ = nullptr;
  usize size_ = 0;

  constexpr bool is_empty() const
  {
    return size_ == 0;
  }

  constexpr T *data() const
  {
    return data_;
  }

  constexpr usize size() const
  {
    return size_;
  }

  constexpr u32 size32() const
  {
    return (u32) size_;
  }

  constexpr u64 size64() const
  {
    return (u64) size_;
  }

  constexpr usize size_bytes() const
  {
    return sizeof(T) * size_;
  }

  constexpr T *begin() const
  {
    return data_;
  }

  constexpr T *end() const
  {
    return data_ + size_;
  }

  constexpr T &operator[](usize index) const
  {
    return data_[index];
  }

  constexpr T &get(usize index) const
  {
    return data_[index];
  }

  template <typename... Args>
  constexpr void set(usize index, Args &&...args) const
    requires(NonConst<T>)
  {
    data_[index] = T{((Args &&) args)...};
  }

  constexpr operator Span<T const>() const
  {
    return Span<T const>{data_, size_};
  }

  constexpr Span<T const> as_const() const
  {
    return Span<T const>{data_, size_};
  }

  constexpr Span<u8 const> as_u8() const
  {
    return Span<u8 const>{reinterpret_cast<u8 const *>(data_), size_bytes()};
  }

  constexpr Span<char const> as_char() const
  {
    return Span<char const>{reinterpret_cast<char const *>(data_),
                            size_bytes()};
  }

  constexpr Span slice(usize offset, usize span) const
  {
    return slice(Slice{offset, span});
  }

  constexpr Span slice(Slice s) const
  {
    s = s(size_);
    return Span{data_ + s.offset, s.span};
  }

  constexpr Span slice(usize offset) const
  {
    return slice(offset, USIZE_MAX);
  }

  template <typename U>
  Span<U> reinterpret() const
  {
    return Span<U>{(U *) data_, (sizeof(T) * size_) / sizeof(U)};
  }
};

template <typename T>
constexpr Span<T const> span(std::initializer_list<T> list)
{
  return Span<T const>{list.begin(), list.size()};
}

template <typename T, usize N>
constexpr Span<T> span(T (&array)[N])
{
  return Span<T>{array, N};
}

template <typename Container>
constexpr auto span(Container &c) -> decltype(Span{data(c), size(c)})
{
  return Span{data(c), size(c)};
}

constexpr Span<char const> operator""_span(char const *lit, usize n)
{
  return Span<char const>{lit, n};
}

constexpr Span<c8 const> operator""_span(c8 const *lit, usize n)
{
  return Span<c8 const>{lit, n};
}

constexpr Span<c16 const> operator""_span(c16 const *lit, usize n)
{
  return Span<c16 const>{lit, n};
}

constexpr Span<c32 const> operator""_span(c32 const *lit, usize n)
{
  return Span<c32 const>{lit, n};
}

inline Span<u8 const> operator""_utf(c8 const *lit, usize n)
{
  return Span<u8 const>{(u8 const *) lit, n};
}

inline Span<u16 const> operator""_utf(c16 const *lit, usize n)
{
  return Span<u16 const>{(u16 const *) lit, n};
}

inline Span<u32 const> operator""_utf(c32 const *lit, usize n)
{
  return Span<u32 const>{(u32 const *) lit, n};
}

constexpr bool get_bit(Span<u8 const> s, usize i)
{
  return get_bit(s[i >> 3], i & 7);
}

constexpr bool get_bit(Span<u16 const> s, usize i)
{
  return get_bit(s[i >> 4], i & 15);
}

constexpr bool get_bit(Span<u32 const> s, usize i)
{
  return get_bit(s[i >> 5], i & 31);
}

constexpr bool get_bit(Span<u64 const> s, usize i)
{
  return get_bit(s[i >> 6], i & 63);
}

constexpr void set_bit(Span<u8> s, usize i)
{
  set_bit(s[i >> 3], i & 7);
}

constexpr void set_bit(Span<u16> s, usize i)
{
  set_bit(s[i >> 4], i & 15);
}

constexpr void set_bit(Span<u32> s, usize i)
{
  set_bit(s[i >> 5], i & 31);
}

constexpr void set_bit(Span<u64> s, usize i)
{
  set_bit(s[i >> 6], i & 63);
}

constexpr void assign_bit(Span<u8> s, usize i, bool b)
{
  assign_bit(s[i >> 3], i & 7, b);
}

constexpr void assign_bit(Span<u16> s, usize i, bool b)
{
  assign_bit(s[i >> 4], i & 15, b);
}

constexpr void assign_bit(Span<u32> s, usize i, bool b)
{
  assign_bit(s[i >> 5], i & 31, b);
}

constexpr void assign_bit(Span<u64> s, usize i, bool b)
{
  assign_bit(s[i >> 6], i & 63, b);
}

constexpr void clear_bit(Span<u8> s, usize i)
{
  clear_bit(s[i >> 3], i & 7);
}

constexpr void clear_bit(Span<u16> s, usize i)
{
  clear_bit(s[i >> 4], i & 15);
}

constexpr void clear_bit(Span<u32> s, usize i)
{
  clear_bit(s[i >> 5], i & 31);
}

constexpr void clear_bit(Span<u64> s, usize i)
{
  clear_bit(s[i >> 6], i & 63);
}

constexpr void flip_bit(Span<u8> s, usize i)
{
  flip_bit(s[i >> 3], i & 7);
}

constexpr void flip_bit(Span<u16> s, usize i)
{
  flip_bit(s[i >> 4], i & 15);
}

constexpr void flip_bit(Span<u32> s, usize i)
{
  flip_bit(s[i >> 5], i & 31);
}

constexpr void flip_bit(Span<u64> s, usize i)
{
  flip_bit(s[i >> 6], i & 63);
}

template <typename T>
constexpr usize impl_find_set_bit(Span<T const> s)
{
  for (usize i = 0; i < s.size(); i++)
  {
    if (s[i] != 0)
    {
      return (i << NumTraits<T>::LOG2_NUM_BITS) | std::countr_zero(s[i]);
    }
  }

  return s.size() << NumTraits<T>::LOG2_NUM_BITS;
}

template <typename T>
constexpr usize impl_find_clear_bit(Span<T const> s)
{
  for (usize i = 0; i < s.size(); i++)
  {
    if (s[i] != NumTraits<T>::MAX)
    {
      return (i << NumTraits<T>::LOG2_NUM_BITS) | std::countr_one(s[i]);
    }
  }

  return s.size() << NumTraits<T>::LOG2_NUM_BITS;
}

constexpr usize find_set_bit(Span<u8 const> s)
{
  return impl_find_set_bit(s);
}

constexpr usize find_set_bit(Span<u16 const> s)
{
  return impl_find_set_bit(s);
}

constexpr usize find_set_bit(Span<u32 const> s)
{
  return impl_find_set_bit(s);
}

constexpr usize find_set_bit(Span<u64 const> s)
{
  return impl_find_set_bit(s);
}

constexpr usize find_clear_bit(Span<u8 const> s)
{
  return impl_find_clear_bit(s);
}

constexpr usize find_clear_bit(Span<u16 const> s)
{
  return impl_find_clear_bit(s);
}

constexpr usize find_clear_bit(Span<u32 const> s)
{
  return impl_find_clear_bit(s);
}

constexpr usize find_clear_bit(Span<u64 const> s)
{
  return impl_find_clear_bit(s);
}

template <typename R>
struct BitSpan
{
  using Type = bool;
  using Repr = R;

  Span<R> repr_     = {};
  usize   bit_size_ = 0;

  constexpr Span<R> repr() const
  {
    return repr_;
  }

  constexpr usize trailing() const
  {
    return repr_.size() - bit_size_;
  }

  constexpr usize size() const
  {
    return bit_size_;
  }

  constexpr bool is_empty() const
  {
    return bit_size_ == 0;
  }

  constexpr bool has_trailing() const
  {
    return bit_size_ != (repr_.size_bytes() * 8);
  }

  constexpr bool operator[](usize index) const
  {
    return ::ash::get_bit(repr_, index);
  }

  constexpr bool get(usize index) const
  {
    return ::ash::get_bit(repr_, index);
  }

  constexpr void set(usize index, bool value) const
  {
    ::ash::assign_bit(repr_, index, value);
  }

  constexpr bool get_bit(usize index) const
  {
    return ::ash::get_bit(repr_, index);
  }

  constexpr bool set_bit(usize index) const
  {
    return ::ash::set_bit(repr_, index);
  }

  constexpr bool clear_bit(usize index) const
  {
    return ::ash::clear_bit(repr_, index);
  }

  constexpr void flip_bit(usize index) const
  {
    ::ash::flip_bit(repr_, index);
  }

  constexpr usize find_set_bit()
  {
    return min(::ash::find_set_bit(repr_), size());
  }

  constexpr usize find_clear_bit()
  {
    return min(::ash::find_clear_bit(repr_), size());
  }

  constexpr operator BitSpan<R const>() const
  {
    return BitSpan<R const>{repr_, bit_size_};
  }

  constexpr BitSpan<R const> as_const() const
  {
    return BitSpan<R const>{repr_, bit_size_};
  }
};

template <typename R>
constexpr BitSpan<R> bit_span(Span<R> span, usize num_bits)
{
  return BitSpan<R>{.repr_ = span, .bit_size_ = num_bits};
}

template <typename Lambda>
struct defer
{
  Lambda lambda;
  constexpr defer(defer &&)                 = delete;
  constexpr defer(defer const &)            = delete;
  constexpr defer &operator=(defer &&)      = delete;
  constexpr defer &operator=(defer const &) = delete;
  constexpr defer(Lambda &&l) : lambda{(Lambda &&) l}
  {
  }
  constexpr ~defer()
  {
    lambda();
  }
};

template <typename Lambda>
defer(Lambda &&) -> defer<Lambda>;

template <typename Sig>
struct Fn;

/// @brief Fn is a type-erased function containing a callback and a pointer. Fn
/// is a reference to both the function to be called and its associated data, it
/// doesn't manage any lifetime.
/// @param dispatcher function/callback to be invoked. typically a
/// dispatcher/thunk.
/// @param data associated data/context for the dispatcher to operate on.
template <typename R, typename... Args>
struct Fn<R(Args...)>
{
  using Dispatcher = R (*)(void *, Args...);

  constexpr R operator()(Args... args) const
  {
    return dispatcher(data, static_cast<Args &&>(args)...);
  }

  Dispatcher dispatcher = nullptr;
  void      *data       = nullptr;
};

template <typename R, typename... Args>
struct PFnDispatcher
{
  static constexpr R dispatch(void *data, Args... args)
  {
    using PFn = R (*)(Args...);

    PFn pfn = reinterpret_cast<PFn>(data);

    return pfn(static_cast<Args &&>(args)...);
  }
};

template <typename Sig>
struct PFnTraits;

template <typename R, typename... Args>
struct PFnTraits<R(Args...)>
{
  using Ptr        = R (*)(Args...);
  using Fn         = ::ash::Fn<R(Args...)>;
  using ReturnType = R;
  using Dispatcher = PFnDispatcher<R, Args...>;
};

template <typename R, typename... Args>
struct PFnTraits<R (*)(Args...)> : PFnTraits<R(Args...)>
{
};

template <typename T, typename R, typename... Args>
struct FunctorDispatcher
{
  static constexpr R dispatch(void *data, Args... args)
  {
    return (*(reinterpret_cast<T *>(data)))(static_cast<Args &&>(args)...);
  }
};

template <class Sig>
struct MemberFnTraits
{
};

/// @brief non-const member function traits
template <class T, typename R, typename... Args>
struct MemberFnTraits<R (T::*)(Args...)>
{
  using Ptr        = R (*)(Args...);
  using Fn         = ::ash::Fn<R(Args...)>;
  using Type       = T;
  using ReturnType = R;
  using Dispatcher = FunctorDispatcher<T, R, Args...>;
};

/// @brief const member function traits
template <class T, typename R, typename... Args>
struct MemberFnTraits<R (T::*)(Args...) const>
{
  using Ptr        = R (*)(Args...);
  using Fn         = ::ash::Fn<R(Args...)>;
  using Type       = T const;
  using ReturnType = R;
  using Dispatcher = FunctorDispatcher<T const, R, Args...>;
};

template <class T>
struct FunctorTraits : MemberFnTraits<decltype(&T::operator())>
{
};

/// @brief make a function view from a raw function pointer.
template <typename R, typename... Args>
auto fn(R (*pfn)(Args...))
{
  using Traits     = PFnTraits<R(Args...)>;
  using Fn         = typename Traits::Fn;
  using Dispatcher = typename Traits::Dispatcher;

  return Fn{&Dispatcher::dispatch, reinterpret_cast<void *>(pfn)};
}

/// @brief make a function view from a captureless lambda/functor (i.e. lambdas
/// without data)
template <typename StaticFunctor>
auto fn(StaticFunctor functor)
{
  using Traits = FunctorTraits<StaticFunctor>;
  using PFn    = typename Traits::Ptr;

  PFn pfn = static_cast<PFn>(functor);

  return fn(pfn);
}

/// @brief make a function view from a functor reference. Functor should outlive
/// the Fn
template <typename Functor>
auto fn(Functor *functor)
{
  using Traits     = FunctorTraits<Functor>;
  using Fn         = typename Traits::Fn;
  using Dispatcher = typename Traits::Dispatcher;

  return Fn{&Dispatcher::dispatch,
            const_cast<void *>(reinterpret_cast<void const *>(functor))};
}

/// @brief create a function view from an object reference and a function
/// dispatcher to execute using the object reference as its first argument.
template <typename T, typename R, typename... Args>
auto fn(T *t, R (*fn)(T *, Args...))
{
  return Fn<R(Args...)>{reinterpret_cast<R (*)(void *, Args...)>(fn),
                        const_cast<void *>(reinterpret_cast<void const *>(t))};
}

/// @brief create a function view from an object reference and a static
/// non-capturing lambda to execute using the object reference as its first
/// argument.
template <typename T, typename StaticFunctor>
auto fn(T *t, StaticFunctor functor)
{
  using Traits = FunctorTraits<StaticFunctor>;
  using PFn    = typename Traits::Ptr;

  PFn pfn = static_cast<PFn>(functor);

  return fn(t, pfn);
}

/// @brief The `SourceLocation`  class represents certain information about the
/// source code, such as file names, line numbers, and function names.
/// Previously, functions that desire to obtain this information about the call
/// site (for logging, testing, or debugging purposes) must use macros so that
/// predefined macros like `__LINE__` and `__FILE__` are expanded in the context
/// of the caller. The `SourceLocation` class provides a better alternative.
///
/// based on: https://en.cppreference.com/w/cpp/utility/source_location
///
struct SourceLocation
{
  static constexpr SourceLocation current(
#if ASH_HAS_BUILTIN(FILE) || (defined(__cpp_lib_source_location) && \
                              __cpp_lib_source_location >= 201907L)
      char const *file = __builtin_FILE(),
#elif defined(__FILE__)
      char const *file = __FILE__,
#else
      char const *file = "unknown",
#endif

#if ASH_HAS_BUILTIN(FUNCTION) || (defined(__cpp_lib_source_location) && \
                                  __cpp_lib_source_location >= 201907L)
      char const *function = __builtin_FUNCTION(),
#else
      char const *function = "unknown",
#endif

#if ASH_HAS_BUILTIN(LINE) || (defined(__cpp_lib_source_location) && \
                              __cpp_lib_source_location >= 201907L)
      u32 line = __builtin_LINE(),
#elif defined(__LINE__)
      u32 line = __LINE__,
#else
      u32 line = 0,
#endif

#if ASH_HAS_BUILTIN(COLUMN) || (defined(__cpp_lib_source_location) && \
                                __cpp_lib_source_location >= 201907L)
      u32 column = __builtin_COLUMN()
#else
      u32 column = 0
#endif
  )
  {
    return SourceLocation{file, function, line, column};
  }

  char const *file     = "";
  char const *function = "";
  u32         line     = 0;
  u32         column   = 0;
};

template <typename T = void>
struct Pin
{
  typedef T Type;

  T v;

  template <typename... Args>
  constexpr Pin(Args &&...args) : v{((Args &&) args)...}
  {
  }
  constexpr Pin(Pin const &)            = delete;
  constexpr Pin(Pin &&)                 = delete;
  constexpr Pin &operator=(Pin const &) = delete;
  constexpr Pin &operator=(Pin &&)      = delete;
  constexpr ~Pin()                      = default;
};

template <>
struct Pin<void>
{
  typedef void Type;

  constexpr Pin()                       = default;
  constexpr Pin(Pin const &)            = delete;
  constexpr Pin(Pin &&)                 = delete;
  constexpr Pin &operator=(Pin const &) = delete;
  constexpr Pin &operator=(Pin &&)      = delete;
  constexpr ~Pin()                      = default;
};

template <typename R>
constexpr void uninit(R &r)
{
  r.uninit();
};

template <typename R>
struct Smart
{
  typedef R Resource;

  constexpr Smart();
  constexpr Smart(Smart const &);
  constexpr Smart(Smart &&);
  constexpr Smart &operator=(Smart const &);
  constexpr Smart &operator=(Smart &&);
  constexpr ~Smart()
  {
    uninit(resource);
  }

  R resource;
};

constexpr u8 sat_add(u8 a, u8 b)
{
  return (((a + b)) < a) ? U8_MAX : (a + b);
}

constexpr u16 sat_add(u16 a, u16 b)
{
  return (((a + b)) < a) ? U16_MAX : (a + b);
}

constexpr u32 sat_add(u32 a, u32 b)
{
  return (((a + b)) < a) ? U32_MAX : (a + b);
}

constexpr u64 sat_add(u64 a, u64 b)
{
  return (((a + b)) < a) ? U64_MAX : (a + b);
}

constexpr i8 sat_add(i8 a, u8 b)
{
  return (i8) clamp((i16) ((i16) a + (i16) b), (i16) I8_MIN, (i16) I8_MAX);
}

constexpr i16 sat_add(i16 a, i16 b)
{
  return (i16) clamp((i32) a + (i32) b, (i32) I16_MIN, (i32) I16_MAX);
}

constexpr i32 sat_add(i32 a, i32 b)
{
  return (i32) clamp((i64) a + (i64) b, (i64) I32_MIN, (i64) I32_MAX);
}

constexpr i64 sat_add(i64 a, i64 b)
{
  if (a > 0)
  {
    b = I64_MAX - a;
  }
  else if (b < (I64_MIN - a))
  {
    b = I64_MIN - a;
  }

  return a + b;
}

// [ ] sat_sub
// [ ] sat_mul
// [ ] sat_div

}        // namespace ash
