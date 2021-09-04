#pragma once

#include <atomic>
#include <cinttypes>
#include <utility>

#include "stx/lock_status.h"
#include "stx/struct.h"

// This is an annotation for multi-threaded locks.
// Any acquired lock must use this.
//
// RULES:
//
// - must not execute user code, i.e. generic type constructors and destructors.
// - operations performed must take constant time and must be extremely
// short-lived. you either be able to say specifically what amount of time it
// takes or not.
//
#define CRITICAL_SECTION(...) \
  do                          \
    __VA_ARGS__               \
  while (false)

namespace stx {

// a rarely-contended lock.
//
// desirable for low-latency scenarios.
// typically used when the operations on the objects being guarded/locked are
// very short.
//
// less desirable for multi-contended and frequently updated memory regions.
struct SpinLock {
  STX_MAKE_PINNED(SpinLock)

  SpinLock() : lock_status{LockStatus::Unlocked} {}

  void lock() {
    LockStatus expected = LockStatus::Unlocked;
    LockStatus target = LockStatus::Locked;
    while (!lock_status.compare_exchange_strong(expected, target,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed)) {
      expected = LockStatus::Unlocked;
    }
  }

  void unlock() {
    lock_status.store(LockStatus::Unlocked, std::memory_order_release);
  }

 private:
  std::atomic<LockStatus> lock_status;
};

template <typename Resource>
struct LockGuard {
  STX_MAKE_PINNED(LockGuard)

  explicit LockGuard(Resource& iresource, char const* operation_name = "")
      : resource{&iresource} {
    (void)operation_name;
    resource->lock();
  }

  ~LockGuard() { resource->unlock(); }

 private:
  Resource* resource;
};

}  // namespace stx

#define WITH_LOCK(lock, ...) \
  CRITICAL_SECTION({         \
    LockGuard guard{lock};   \
                             \
    __VA_ARGS__              \
  })
