#pragma once

#define ASH_DISABLE_COPY(target_type)                   \
  target_type(target_type const &)            = delete; \
  target_type &operator=(target_type const &) = delete;

#define ASH_DISABLE_MOVE(target_type)              \
  target_type(target_type &&)            = delete; \
  target_type &operator=(target_type &&) = delete;

#define ASH_DISABLE_DEFAULT_CONSTRUCTOR(target_type) target_type() = delete;
#define ASH_DISABLE_DEFAULT_DESTRUCTOR(target_type) ~target_type() = delete;

#define ASH_DEFAULT_CONSTRUCTOR(target_type) target_type() = default;
#define ASH_DEFAULT_DESTRUCTOR(target_type) ~target_type() = default;

#define ASH_DEFAULT_COPY(target_type)                    \
  target_type(target_type const &)            = default; \
  target_type &operator=(target_type const &) = default;

#define ASH_DEFAULT_MOVE(target_type)               \
  target_type(target_type &&)            = default; \
  target_type &operator=(target_type &&) = default;

/// used for ensuring that the referred-to object remains pinned to its memory
/// address and its contents are not moved or copied as its content could also
/// refer to the object itself. this enforces stability of the address of the
/// object as it will not change throughout its lifetime.
//
// This also means its content can not be duplicated in anyway. i.e. structs in
// which it doesn't make sense to copy their bytes.
//
#define ASH_MAKE_PINNED(target_type) \
  ASH_DISABLE_COPY(target_type)      \
  ASH_DISABLE_MOVE(target_type)
