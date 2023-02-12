#pragma once

#include "spdlog/spdlog.h"
#include "stx/option.h"
#include "stx/panic.h"

#define ASH_PANIC(...) ::stx::panic(__VA_ARGS__)

#define ASH_CHECK(expr, ...)                \
  do {                                      \
    if (!(expr)) ::stx::panic(__VA_ARGS__); \
  } while (false)

#define ASH_LOG(...)             \
  do {                           \
    ::spdlog::info(__VA_ARGS__); \
  } while (false)

#define ASH_LOG_IF(expr, ...)                \
  do {                                       \
    if ((expr)) ::spdlog::info(__VA_ARGS__); \
  } while (false)

#define ASH_LOG_WARN(...)        \
  do {                           \
    ::spdlog::warn(__VA_ARGS__); \
  } while (false)

#define ASH_LOG_ERR(...)          \
  do {                            \
    ::spdlog::error(__VA_ARGS__); \
  } while (false)

#define ASH_LOG_TRACE(...)        \
  do {                            \
    ::spdlog::trace(__VA_ARGS__); \
  } while (false)

#define ASH_LOG_WARN_IF(expr, ...)           \
  do {                                       \
    if ((expr)) ::spdlog::warn(__VA_ARGS__); \
  } while (false)

#define ASH_ERRNUM_CASE(x) \
  case x:                  \
    return #x;

#define ASH_UNREACHABLE() \
  ASH_PANIC("Expected program execution to not reach this state")

#define AS_U8(...) static_cast<ash::u8>(__VA_ARGS__)
#define AS_U16(...) static_cast<ash::u16>(__VA_ARGS__)
#define AS_U32(...) static_cast<ash::u32>(__VA_ARGS__)
#define AS_U64(...) static_cast<ash::u64>(__VA_ARGS__)

#define AS_I8(...) static_cast<ash::i8>(__VA_ARGS__)
#define AS_I16(...) static_cast<ash::i16>(__VA_ARGS__)
#define AS_I32(...) static_cast<ash::i32>(__VA_ARGS__)
#define AS_I64(...) static_cast<ash::i64>(__VA_ARGS__)

#define AS_F32(...) static_cast<ash::f32>(__VA_ARGS__)
#define AS_F64(...) static_cast<ash::f64>(__VA_ARGS__)

#define AS(type, ...) static_cast<type>(__VA_ARGS__)

namespace ash {

template <typename Target, typename Source>
STX_FORCE_INLINE stx::Option<Target *> upcast(Source &source) {
  Target *ptr = dynamic_cast<Target *>(&source);
  if (ptr == nullptr) {
    return stx::None;
  } else {
    return stx::Some(static_cast<Target *>(ptr));
  }
}

}  // namespace ash
