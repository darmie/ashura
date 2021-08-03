#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "stx/option.h"
#include "stx/result.h"
#include "vlk/utils.h"

// exception-safety: absolute zero
// we don't use exceptions and neither do we plan to support it

namespace vlk {

// source:
// https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │
// ...
constexpr size_t hardware_constructive_interference_size =
    2 * sizeof(std::max_align_t);
constexpr size_t hardware_destructive_interference_size =
    2 * sizeof(std::max_align_t);
#endif

// each CPU core has its cache line, cache lines optimize for reading and
// writing to main memory which is slow. while multi-threading or using async,
// we need to communicate across threads which could map to CPU cores. the
// memory addresses are shared across CPU cores, as such we need to ensure we
// are not performing false sharing across these cores.
//
// False sharing leads to excessive cache flushes and thus reduces
// multi-threaded performances as the CPU now has to read from main memory which
// is the slowest read path. false sharing happens along word boundaries which
// is the individual unit of reading from memory. i.e. on a 64-bit system. 8
// uint8-s might be packed by the compiler into a single word (uint64), sharing
// atomics of uint8 along this word boundary would lead to excessive flushing
// across each CPU core's cache line on write to the cache line of either core.
//
// A ripple effect as each CPU core's cache line entry for the cached address of
// the uint8-s has now been invalidated and each CPU core's cache now has to
// reload from main memory
template <typename ValueType>
struct CacheLineAligned {
  alignas(hardware_destructive_interference_size) ValueType v;
};

enum class [[nodiscard]] LockStatus : uint8_t{Unlocked, Locked};

/// the future's status are mutually exclusive. i.e. no two can exist at once.
/// and some states might be skipped or never occur or observed during the async
/// operation.
///
/// Implementation Note: this enum is typically used in relaxed memory order. It
/// is only used in release memory order if it enters the `Completed` state and
/// the executor makes non-atomic changes within the task's scope i.e. setting a
/// completion result to the shared future state object.
///
/// future status are updated only by the executor.
///
enum class [[nodiscard]] FutureStatus : uint8_t{
    /// the async operation has not been scheduled for execution.
    /// default initial state of a newly created future. a re-cycled future
    /// might
    /// not observe this state.
    ///
    /// REQUIRED STATE?: Yes. this is the default-initialized state of the
    /// future,
    /// a recycled future must transition to this state.
    ///
    /// INTENDED FOR: executors that wish to notify of task scheduling state.
    ///
    Unscheduled,
    /// the async operation has been submitted to the scheduler and is scheduled
    /// for execution.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// scheduling.
    ///
    /// INTENDED FOR:
    ///
    Scheduled,
    /// the async operation has been submitted by the scheduler to the executor
    /// for
    /// execution.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor has a task
    /// scheduler. i.e. an immediately-executing executor doesn't need
    /// submission.
    ///
    /// INTENDED FOR:
    ///
    Submitted,
    /// the async operation is now being executed by the executor.
    /// this can also mean that the task has been resumed from the suspended or
    /// force suspended state.
    ///
    /// REQUIRED STATE?: No, can be skipped.
    ///
    /// INTENDED FOR: executors that wish to notify of task execution. an
    /// immediately-executing executor might need to avoid the nanosecond
    /// overhead
    /// of an atomic operation (i.e. notification of the Future's state).
    ///
    Executing,
    /// the async operation is now being canceled due to a cancelation request.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// cancelation and cancelation has been requested.
    ///
    /// INTENDED FOR: cancelable executors with prolonged or staged cancelation
    /// procedures.
    ///
    Canceling,
    /// the async operation is now being forced to cancel by the executor. this
    /// happens without the user requesting for it. i.e. the scheduler and
    /// execution context might want to shutdown and then cancel all pending
    /// tasks.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// cancelation and cancelation has been forced by the executor.
    ///
    /// `INTENDED FOR`: cancelable executors with prolonged or staged
    /// cancelation procedures.
    ///
    ForceCanceling,
    /// the async operation is now being suspended.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// suspension and suspension has been requested.
    ///
    /// INTENDED FOR: suspendable executors with prolonged or staged suspension
    /// procedures.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be preceded by the `Executing` state.
    ///
    Suspending,
    /// the async operation is now being forced to suspend.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// suspension and suspension has been forced by the executor.
    ///
    /// INTENDED FOR: suspendable executors with prolonged or staged suspension
    /// procedures.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be preceded by the `Executing` and
    /// `ForceSuspending` states.
    ///
    // TODO(lamarrr): what about preemption?
    ForceSuspending,
    /// the async operation has been suspended.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// suspension and suspension has been requested.
    ///
    /// INTENDED FOR: suspendable executors.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be preceded by the `Suspending` and
    /// `Executing`
    /// states.
    ///
    Suspended,
    /// the async operation has been forcefully suspended.
    ///
    /// `REQUIRED STATE?`: No, can be skipped. Set only if the executor supports
    /// suspension and suspension has forced by the executor.
    ///
    /// `INTENDED FOR`: suspendable executors.
    ///
    /// `IMPLEMENTATION REQUIREMENT`: must be preceded by the `ForceSuspending`
    /// and `Executing` states.
    ///
    ForceSuspended,
    /// the async operation is being resumed.
    ///
    /// `REQUIRED STATE?`: No, can be skipped. Set only if the executor supports
    /// suspension and resumption has been requested.
    ///
    /// INTENDED FOR: executors with prolonged or staged resumption procedure.
    ///
    /// IMPLEMENTATION REQUIREMENT: must preceded by the `Executing` and
    /// `Suspending`
    /// states.
    ///
    Resuming,
    /// the async operation is being forcefully resumed.
    ///
    /// `REQUIRED STATE?`: No, can be skipped. Set only if the executor supports
    /// suspension and needs to force the async operation into resumption, i.e.
    /// a prioritizing scheduler.
    ///
    /// `INTENDED FOR`: executors with prolonged or staged resumption procedure.
    ///
    /// `IMPLEMENTATION REQUIREMENT`: must be preceded by the `Executing`,
    /// `ForceSuspending`, and `ForceSuspended` states.
    ///
    ForceResuming,
    /// the async operation has been canceled.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// cancelation and cancelation has been requested.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be a terminal state for cancelable
    /// executors.
    ///
    Canceled,
    /// the async operation has been forcefully canceled.
    ///
    /// REQUIRED STATE?: No, can be skipped. Set only if the executor supports
    /// cancelation and cancelation has been forced by the executor.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be a terminal state for cancelable
    /// executors.
    ///
    ForceCanceled,
    /// the async operation has been completed.
    ///
    /// REQUIRED STATE?: Yes, if async operation is complete-able. must be set
    /// once
    /// the async operation has been completed.
    /// this implies that completion is not required i.e. a forever-running task
    /// that never completes.
    ///
    /// IMPLEMENTATION REQUIREMENT: must be a terminal state for executors on
    /// complete-able tasks.
    ///
    Completed};

enum class TaskPriority : uint32_t {};

enum class TaskPriorityHint : std::underlying_type_t<TaskPriority> {
  Background = 0,
  Interactive = 1
};

/// the executor might not be able to immediately respond to the requested
/// states of the async operations. the executor might not even be able to
/// attend to it at all.
enum class [[nodiscard]] CancelRequestedState : uint8_t{Uncanceled, Canceled};

/// the executor might not be able to immediately respond to the requested
/// states of the async operations. the executor might not even be able to
/// attend to it at all. this implies that if the user requests for resumption
/// of the task and immediately requests for suspension, the last requested
/// state takes effect and is the state observed by the executor.
///
enum class [[nodiscard]] SuspendRequestedState : uint8_t{Resumed, Suspended};

// this struct helps gurantee ordering of instructions relative
// to the future's shared state. it doesn't guarentee ordering of
// instructions relative to the function scope or program state itself. or even
// the async operation's associated task, the user has to take care of that
// themselves.
//
struct FutureBaseState {
  VLK_DEFAULT_CONSTRUCTOR(FutureBaseState)
  STX_DISABLE_COPY(FutureBaseState)
  VLK_DISABLE_MOVE(FutureBaseState)

  void notify_unscheduled() {
    future_status.v.store(FutureStatus::Unscheduled, std::memory_order_relaxed);
  }

  void notify_scheduled() {
    future_status.v.store(FutureStatus::Scheduled, std::memory_order_relaxed);
  }

  void notify_submitted() {
    future_status.v.store(FutureStatus::Submitted, std::memory_order_relaxed);
  }

  void notify_executing() {
    future_status.v.store(FutureStatus::Executing, std::memory_order_relaxed);
  }

  CancelRequestedState fetch_requested_cancel_state() const {
    return cancel_requested_state.v.load(std::memory_order_relaxed);
  }

  SuspendRequestedState fetch_requested_suspend_state() const {
    return suspend_requested_state.v.load(std::memory_order_relaxed);
  }

  void notify_attended_to_cancel_request() {
    future_status.v.store(FutureStatus::Canceled, std::memory_order_relaxed);
  }

  void notify_attended_to_suspend_request() {
    future_status.v.store(FutureStatus::Suspended, std::memory_order_relaxed);
  }

  void notify_attended_to_resume_request() {
    future_status.v.store(FutureStatus::Executing, std::memory_order_relaxed);
  }

  void notify_force_canceling() {
    future_status.v.store(FutureStatus::ForceCanceling,
                          std::memory_order_relaxed);
  }

  void notify_force_suspending() {
    future_status.v.store(FutureStatus::ForceSuspending,
                          std::memory_order_relaxed);
  }

  void notify_force_resuming() {
    future_status.v.store(FutureStatus::ForceResuming,
                          std::memory_order_relaxed);
  }

  void notify_force_canceled() {
    future_status.v.store(FutureStatus::ForceCanceled,
                          std::memory_order_relaxed);
  }

  void notify_force_suspended() {
    future_status.v.store(FutureStatus::ForceSuspended,
                          std::memory_order_relaxed);
  }

  void notify_force_resumed() {
    future_status.v.store(FutureStatus::Executing, std::memory_order_relaxed);
  }

  void notify_completed_with_no_return_value() {
    future_status.v.store(FutureStatus::Completed, std::memory_order_relaxed);
  }

  // sends that the async operation has completed and the shared value storage
  // has been updated, so it can begin reading from it
  void notify_completed_with_return_value() {
    future_status.v.store(FutureStatus::Completed, std::memory_order_release);
  }

  FutureStatus fetch_status() const {
    return future_status.v.load(std::memory_order_relaxed);
  }

  [[nodiscard]] bool is_done() const {
    switch (fetch_status()) {
      case FutureStatus::Completed:
      case FutureStatus::Canceled:
      case FutureStatus::ForceCanceled: {
        return true;
      }

      case FutureStatus::Unscheduled:
      case FutureStatus::Scheduled:
      case FutureStatus::Submitted:
      case FutureStatus::Executing:
      case FutureStatus::Canceling:
      case FutureStatus::ForceCanceling:
      case FutureStatus::Suspending:
      case FutureStatus::ForceSuspending:
      case FutureStatus::Resuming:
      case FutureStatus::ForceResuming: {
        return false;
      }
    }
  }

  // acquires write operations and stored value that happend on the
  // executor thread.
  FutureStatus fetch_status_with_result() const {
    return future_status.v.load(std::memory_order_acquire);
  }

  void request_cancel() {
    cancel_requested_state.v.store(CancelRequestedState::Canceled,
                                   std::memory_order_relaxed);
  }

  void request_resume() {
    // NOTE: suspension might still be on the queue, but we are not informing
    // the user about that. so both are canceled.
    // if the user suspends for resume and suspension immediately before the
    // executor has an opportunity to respond, we ignore both requests since
    // they cancel each other.
    //
    suspend_requested_state.v.store(SuspendRequestedState::Resumed,
                                    std::memory_order_relaxed);
  }

  void request_suspend() {
    // NOTE: resumption might still be on the queue, but we are not informing
    // the user about that.
    // if the user suspends for resume and suspension immediately before the
    // executor has an opportunity to respond, we ignore both requests since
    // they cancel each other.
    //
    suspend_requested_state.v.store(SuspendRequestedState::Suspended,
                                    std::memory_order_relaxed);
  }

 private:
  CacheLineAligned<std::atomic<FutureStatus>> future_status{
      FutureStatus::Unscheduled};
  CacheLineAligned<std::atomic<CancelRequestedState>> cancel_requested_state{
      CancelRequestedState::Uncanceled};
  CacheLineAligned<std::atomic<SuspendRequestedState>> suspend_requested_state{
      SuspendRequestedState::Resumed};
};


struct PromiseBaseState{


  CacheLineAligned<std::atomic<CancelRequestedState>> cancel_requested_state{
      CancelRequestedState::Uncanceled};
  CacheLineAligned<std::atomic<SuspendRequestedState>> suspend_requested_state{
      SuspendRequestedState::Resumed};
};


enum class FutureError : uint8_t { Pending, Canceled };

// TODO(lamarrr): test for const arg
// TODO(lamarrr): does stx's Result handle const?
//
template <typename T>
struct FutureState : public FutureBaseState {
  using MutT = std::remove_const_t<T>;
  using ConstT = std::add_const_t<T>;

  // NOTE: we don't use mutexes on the final result of the async operation
  // since the executor will have exclusive access to the storage address
  // until the async operation is finished (completed, force canceled, or,
  // canceled).
  //
  // note that the async operation's result will be discarded if the
  // future has been discarded.
  //
  std::aligned_storage_t<sizeof(T), alignof(T)> storage;

  stx::Result<T, FutureError> copy() const {
    switch (fetch_status_with_result()) {
      case FutureStatus::Completed: {
        return stx::Ok(unsafe_copy());
      }

      case FutureStatus::Canceled:
      case FutureStatus::ForceCanceled: {
        return stx::Err(FutureError::Canceled);
      }

      case FutureStatus::Unscheduled:
      case FutureStatus::Scheduled:
      case FutureStatus::Submitted:
      case FutureStatus::Executing:
      case FutureStatus::Canceling:
      case FutureStatus::ForceCanceling:
      case FutureStatus::Suspending:
      case FutureStatus::ForceSuspending:
      case FutureStatus::Suspended:
      case FutureStatus::ForceSuspended:
      case FutureStatus::Resuming:
      case FutureStatus::ForceResuming: {
        return stx::Err(FutureError::Pending);
      }
    }
  }

  stx::Result<T, FutureError> move() {
    switch (fetch_status_with_result()) {
      case FutureStatus::Completed: {
        return stx::Ok(unsafe_move());
      }

      case FutureStatus::Canceled:
      case FutureStatus::ForceCanceled: {
        return stx::Err(FutureError::Canceled);
      }

      case FutureStatus::Unscheduled:
      case FutureStatus::Scheduled:
      case FutureStatus::Submitted:
      case FutureStatus::Executing:
      case FutureStatus::Canceling:
      case FutureStatus::ForceCanceling:
      case FutureStatus::Suspending:
      case FutureStatus::ForceSuspending:
      case FutureStatus::Suspended:
      case FutureStatus::ForceSuspended:
      case FutureStatus::Resuming:
      case FutureStatus::ForceResuming: {
        return stx::Err(FutureError::Pending);
      }
    }
  }

  ~FutureState() {
    switch (fetch_status_with_result()) {
      case FutureStatus::Completed: {
        unsafe_destroy();
        return;
      }
      default:
        return;
    }
  }

 private:
  MutT& launder_writable() {
    return *std::launder(reinterpret_cast<MutT*>(&storage));
  }

  T const& launder_readable() const {
    return *std::launder(reinterpret_cast<ConstT*>(&storage));
  }

  // sends in the result of the async operation.
  // calling this function implies that the async operation has completed.
  void unsafe_send(T&& value) { new (&storage) T{std::move(value)}; }

  // copies the result of the async operation.
  // calling this function implies that the async operation has been completed
  // and `unsafe_send()` has been called.
  T unsafe_copy() const { return T{launder_readable()}; }

  // moves out the result of the async operation.
  // calling this function implies that the async operation has been completed
  // and `unsafe_send` has been called. by the C++ object model, the object is
  // left in a valid but unspecified state. after the move, the object state
  // will determined by the object's move constructor. subsequent moves and
  // copies' validity will be affected by this.
  T unsafe_move() { return T{std::move(launder_writable())}; }

  void unsafe_destroy() { launder_writable().~MutT(); }
};

struct InitTagT {};
constexpr InitTagT init_tag{};

template <>
struct FutureState<void> : public FutureBaseState {};

// the user can considering using sync , guards, sink or other ordering and
// synchronization primitives provided by us and the standard library.
//
///
///
///
//
// observes termination of an async operation.
//
// and ensures ordering of instructions or observation of the changes from
// another thread.
//
// this is contrary to the on-finished callback approach in which the user is
// very likely to use incorrectly due instruction re-ordering or order of
// observation of changes.
//
// any side-effects made by the callback function (reference capture or program
// state modifications) must not be observed until `is_finished` returns true or
// `await_finish` is called. this means that we don't require exclusive locking
// of the values being modified (i.e. using a mutex).
//
//
//
// the captured reference's memory address should also be aligned as
// `std::hardware_destructive_interference_size` to prevent cache coherency
// issues.
//
//
//
// this also means only one task must capture the referenced values. otherwise,
// exclusive locking is required to ensure that the values aren't being written
// to across different worker threads.
//
// for canceling a task and submitting a new one, the user has to use the
// cancelation token and call `await_finish` which will block the calling thread
// until the task's cancelation is acknowledged, or the user has to specifically
// use a mutex to lock the captured references to ensure multiple worker threads
// don't write to it at once.
//
// this helps prevent the user from writing ugly hacks like
// `std::shared_ptr<std::atomic<TaskStatus>>` which they might not even use
// correctly.
// this also prevents having the user from manually writing code to track state
// of each submitted task.
//
template <typename T>
struct Future {
  template <typename P>
  friend struct Promise;
  friend struct FutureToken;
  friend struct RequestProxy;

  static auto create() { return Future{init_tag}; }

  Future() : state{nullptr} {}

  explicit Future(InitTagT) : state{new FutureState<T>{}} {}

  // TODO(lamarrr): IMPORTANT:::::::::::::::: a control block is till allocated
  // for a shared ptr with custom destructor. we therefore need handles.
  // handle to a future state
  // static auto recycle() {} // can't really recycle due to the invalid use
  // of????

  FutureStatus fetch_status() const { return state->fetch_status(); }

  void request_cancel() const { state->request_cancel(); }

  void request_suspend() const { state->request_suspend(); }

  void request_resume() const { state->request_resume(); }

  stx::Result<T, FutureError> copy() const { return state->copy(); }

  stx::Result<T, FutureError> move() const { return state->move(); }

  [[nodiscard]] bool is_done() const { return state->is_done(); }

  [[nodiscard]] bool is_valid() const { return state != nullptr; }

 private:
  std::shared_ptr<FutureState<T>> state;
};

template <>
struct Future<void> {
  template <typename P>
  friend struct Promise;
  friend struct FutureToken;
  friend struct RequestProxy;

  static auto create() { return Future{init_tag}; }

  Future() : state{nullptr} {}

  explicit Future(InitTagT) : state{new FutureState<void>{}} {}

  void request_cancel() const { state->request_cancel(); }

  void request_suspend() const { state->request_suspend(); }

  void request_resume() const { state->request_resume(); }

  [[nodiscard]] bool is_done() const { return state->is_done(); }

  [[nodiscard]] bool is_valid() const { return state != nullptr; }

 private:
  std::shared_ptr<FutureState<void>> state;
};

struct FutureToken {
  VLK_DEFAULT_CONSTRUCTOR(FutureToken)

  template <typename T>
  explicit FutureToken(Future<T> const& future)
      : state{std::weak_ptr<FutureBaseState>{
            std::static_pointer_cast<FutureBaseState>(future.state)}} {}

  void request_cancel() const { state->request_cancel(); }

  void request_suspend() const { state->request_suspend(); }

  void request_resume() const { state->request_resume(); }

  [[nodiscard]] bool is_done() const { return state->is_done(); }

  [[nodiscard]] bool is_valid() const { return state != nullptr; }

 private:
  // this is aligned to the cache line size via heap allocation which should
  // make allocations chunked to std::max_align_t granularity. so we shouldn't
  // have cache coherence issues.
  std::shared_ptr<FutureBaseState> state;
};

struct RequestProxy {
  template <typename T>
  explicit RequestProxy(Future<T> const& future)
      : state{std::static_pointer_cast<FutureBaseState>(future.state)} {}

  // TODO(lamarrr): don't cancel if shared_ptr is unavailable????
  //
  //
  CancelRequestedState fetch_requested_cancel_state() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      // the task is canceled since the associated
      // future has been discarded
      // TODO(lamarrr): is this really good?????
      return CancelRequestedState::Uncanceled;
    } else {
      return shared_state->fetch_requested_cancel_state();
    }
  }

  SuspendRequestedState fetch_requested_suspend_state() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      return SuspendRequestedState::Resumed;
    } else {
      return shared_state->fetch_requested_suspend_state();
    }
  }

 private:
  std::weak_ptr<FutureBaseState> state;
};

// TODO(lamarrr): promise should inherit from CancelationProx?

// notifications and results do not propagate if the future has been discarded
template <typename T>
struct Promise {
  explicit Promise(Future<T> const& future)
      : state{future.state}, request_proxy{future} {}

  void finish(T&& value) const {
    auto shared_state = state.lock();

    if (shared_state == nullptr) {
      // the user is no longer interested in the result as they've discarded the
      // future and the future is no longer existing
      return;
    } else {
      switch (shared_state->fetch_cancelation_status()) {
        case CancelationStatus::Canceling: {
          shared_state->release_canceled_state();
        } break;
        case CancelationStatus::CancelRequested:
        case CancelationStatus::Uncanceled: {
          shared_state->unsafe_init(std::move(value));
          return;
        }
      }
    }
  }

  // returns true if the associated future is alive and the task has been
  // requested to suspend.
  // the task is still executed but requested to cancel if the associated
  // future is discarded.
  [[nodiscard]] bool try_acknowledge_suspend() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      return false;
    } else {
      return shared_state->try_acknowledge_suspend();
    }
  }

  // returns true if the associated future is alive and the task has been
  // requested to resume.
  // the task is still executed but requested to cancel if the associated
  // future is discarded. the task is forced into a resumed state to enable
  // proper completion or cancelation (if any).
  [[nodiscard]] bool try_acknowledge_resume() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      return true;
    } else {
      return shared_state->try_acknowledge_resume();
    }
  }

  RequestProxy const& as_cancelation_proxy() const { return cancelation_proxy; }

  // this helps a lot when the future is detached
  bool future_detached() const;

 private:
  std::weak_ptr<FutureState<T>> state;
  RequestProxy request_proxy;
};

template <>
struct Promise<void> {
  explicit Promise(Future<void> const& future)
      : state{future.state}, cancelation_proxy{future} {}

  void finish() const {
    auto shared_state = state.lock();

    if (shared_state == nullptr) {
      // the user is no longer interested in the result as they've discarded the
      // future and the future is no longer existing
      return;
    } else {
      switch (shared_state->fetch_cancelation_status()) {
        case CancelationStatus::Canceling: {
          shared_state->release_canceled_state();
        } break;
        case CancelationStatus::CancelRequested:
        case CancelationStatus::Uncanceled: {
          shared_state->release_completed_state();
          return;
        }
      }
    }
  }

  [[nodiscard]] bool try_acknowledge_suspend() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      return false;
    } else {
      return shared_state->try_acknowledge_suspend();
    }
  }

  [[nodiscard]] bool try_acknowledge_resume() const {
    auto shared_state = state.lock();
    if (shared_state == nullptr) {
      return true;
    } else {
      return shared_state->try_acknowledge_resume();
    }
  }

  bool future_detached() const {}

  CancelationProxy const& as_cancelation_proxy() const {
    return cancelation_proxy;
  }

 private:
  std::weak_ptr<FutureState<void>> state;
  CancelationProxy cancelation_proxy;
};

struct ThreadInfo {
  uint32_t index = 0;
};

struct TaskIdentifier {
  std::shared_ptr<std::string const> ptr;

  std::string_view get_string_view() const {
    if (ptr == nullptr) return "Unnamed Task";
    return *ptr;
  }
};

// TODO(lamarrr): document weak_ptr recommendation
template <typename ReturnType>
struct Task {
  // used to store the function that needs execution. this function needs to be
  // thread-safe.
  std::function<ReturnType(CancelationProxy const&)> function =
      [](CancelationProxy const&) {};
  TaskIdentifier identifier;
};

// result of one is passed to the other
template <typename... FunctionTypes>
struct TaskChain {
  std::tuple<FunctionTypes...> chain;
};

// Parallel?????????????????????

// all executed sequentially without result dependency
template <typename... SubtaskTypes>
struct TaskSequence {
  std::tuple<SubtaskTypes...> tasks;
};

struct PackagedTask {
  // function to be executed in the execution context
  std::function<void()> function = []() {};
  TaskIdentifier identifier;

  VLK_DEFAULT_CONSTRUCTOR(PackagedTask)
  STX_DISABLE_COPY(PackagedTask)
  STX_DEFAULT_MOVE(PackagedTask)

  // TODO(lamarrr): the user might assume fire and forget, cancelation needs to
  // be explicit or by the executor.
  //
  // force cancelation of the task if it is not alive
  //
  template <typename T>
  PackagedTask(Task<T>&& task, Future<T> const& future)
      : function{[promise = Promise<T>(future),
                  task_function = std::move(task.function)]() {
          // it might take time before the executor context's threads are
          // available for executing tasks, so we still have to check for
          // cancelation before beginning to execute any work
          CancelationProxy const& cancelation_proxy =
              promise.as_cancelation_proxy();
          if (cancelation_proxy.try_acknowledge_cancel()) {
          } else {
            auto completion_result = task_function(cancelation_proxy);
            promise.finish(std::move(completion_result));
          }
        }},
        identifier{std::move(task.identifier)} {}

  PackagedTask(Task<void>&& task, Future<void> const& future)
      : function{[promise = Promise<void>(future),
                  task_function = std::move(task.function)]() {
          CancelationProxy const& cancelation_proxy =
              promise.as_cancelation_proxy();
          if (cancelation_proxy.try_acknowledge_cancel()) {
          } else {
            task_function(cancelation_proxy);
            promise.finish();
          }
          //
          // TODO(lamarrr): use task_moved.identifier for tracing
          // we need a weak ptr to the trace context needed for logging so in
          // case the scheduler is destroyed, we'll be able to detect
        }},
        identifier{std::move(task.identifier)} {}
};

namespace impl {

// executed together on same thread, results are gathered
void chain();
// executed on different threads, but in the sequential order
void sequence();

void weave();

// TODO(lamarrr): we want to chain return types so we don't have to perform
// extra allocations for each task
//
// no memory allocations
template <typename ReturnType, typename... TaskTypes>
void then(Task<ReturnType>&&, TaskChain<TaskTypes...>&& chain) {}

// task to task chain
// task chain to task chain
// task to task
void then();
}  // namespace impl

struct ThreadExecutionContext {
  // must be thread-safe
  virtual void submit(PackagedTask&& task) { VLK_PANIC("Unimplemented"); }

  // must be thread-safe

  virtual ~ThreadExecutionContext() {}

  virtual void trace_begin();
  virtual void trace_end();

  // std::min(1, <int32_t>(num_hw_threads)-1 )
  auto get_num_hardware_threads() const { return num_hardware_threads; }

 private:
  uint32_t num_hardware_threads = 0;
};

struct ThreadTaskTrace {};

template <typename RootTaskType, typename NewTaskType>
TaskChain<RootTaskType, NewTaskType> then(Task<RootTaskType>&& first_task,
                                          Task<NewTaskType>&& task);

// TODo(LAMARRR): separate the template parameters
template <typename... TaskTypes>
TaskChain<TaskTypes..., void> then(TaskChain<TaskTypes...>&& first_task,
                                   Task<void>&& task);

namespace impl {
inline void backoff_spin(uint64_t iteration) {
  if (iteration < 64) {
    // immediate spinning
  } else if (iteration < 128) {
    // if there are any threads that need execution, let them execute before
    // attending to us
    std::this_thread::yield();
  } else {
    // sleep for a specific amount of time
    std::this_thread::sleep_for(std::chrono::milliseconds(125));
  }
}

// advisable to never block any thread
//
// this should never be used anywhere in user code
//
template <typename Predicate>
void block_on(Predicate&& predicate) {
  uint64_t uneventful_iterations = 0;
  while (!predicate()) {
    uneventful_iterations++;
    impl::backoff_spin(uneventful_iterations);
  }
}

// TODO(lamarrr): cleanup
//
// acquire exclusive access, non-blocking
inline bool try_acquire_lock(std::atomic<LockStatus>& lock) {
  LockStatus expected = LockStatus::Unlocked;
  return lock.compare_exchange_strong(expected, LockStatus::Locked,
                                      std::memory_order_acquire,
                                      std::memory_order_relaxed);
}

// acquire exclusive access, blocking
inline void block_on_acquire_lock(std::atomic<LockStatus>& lock) {
  block_on([&lock] { return try_acquire_lock(lock); });
}

// release exclusive access, must have been acquired
void release_lock(std::atomic<LockStatus>& lock) {
  lock.store(LockStatus::Unlocked, std::memory_order_release);
}

template <typename FuncSucc, typename FuncFail>
auto try_acquire_then(std::atomic<LockStatus>& lock,
                      FuncSucc&& on_acquire_succeed,
                      FuncFail&& on_acquire_failed) {
  if (try_acquire_lock(lock)) {
    auto result = static_cast<FuncSucc&&>(on_acquire_succeed)();
    release_lock(lock);
    return std::move(result);
  } else {
    return static_cast<FuncFail&&>(on_acquire_failed)();
  }
}

}  // namespace impl

template <typename ReturnType>
void block_on(Future<ReturnType> const& future) {
  impl::block_on([&future]() { return future.is_done(); });
}

// TODO(lamarrr): package task function

// TODO(lamarrr): how do we handle this scenario? submit task, cancel, submit
// another, we don't want to have to wait till the canceled one finishes as it
// could be modifying a value and not respond to the cancelation request.
//
//
// All async operations in the application should be cancelable, the async
// operations should try to be cancelable but are not enforced to.
//
//
// on handling cancelation: if we were to return a future with a result, we
// would need to
//
//
// once scheduler is dropped, all pending tasks would be requested to cancel
//
struct ThreadTaskScheduler {
  //
  //
  // even if the user requests for cancelation and we are unable to service the
  // request, we still have to
  //
  //
  //
  // we can't stop the function from running to completion even if a cancelation
  // is requested
  //
  // tasks are submitted for execution in a FIFO order
  // TODO(lamarrr): need to be able to submit from multiple threads
  // should support submitting one without completion observer or cancelation?
  // a fire-and-forget-style is almost impossible to use
  //
  // discarding the completion observer means you are no longer interested in
  // the result of the computation
  // TODO(lamarrr): this should be a specialization of one that returns void

  template <typename T>
  Future<T> schedule(Task<T>&& task) {
    auto future = Future<T>::create();

    execution_context_->submit(PackagedTask{std::move(task), future});

    return std::move(future);
  }

  // of-course, the long names are meant to discourage you from using them
  //
  // the executor will still have cancelation tokens to execute on the worker
  // threads
  //

  template <typename T, typename DurationRep, typename DurationPeriod>
  Future<T> schedule_every(
      Task<T>&& task,
      std::chrono::duration<DurationRep, DurationPeriod> duration);

  template <typename DurationRep, typename DurationPeriod>
  Future<void> schedule_every(
      Task<void>&& task,
      std::chrono::duration<DurationRep, DurationPeriod> duration);

  // TODO(lamarrr): how to manage lifetimes, we need to shutdown scheduler and
  // ensure none of the tasks are running at the moment we want to shutdown.
  // we'd need a shared_ptr queue of pending taskss
  void attach_execution_context(
      std::shared_ptr<ThreadExecutionContext> execution_context) {}

  bool has_execution_context() const { return execution_context_ != nullptr; }

  void check_execution_context() const {
    VLK_ENSURE(
        has_execution_context(),
        "Task execution context has not been attached to task scheduler");
  }

  // cancel pending when destruction requested
  // store or track cancelation tokens? we should give the physical executor
  // cancelation tokens as part of the packaged task, ensures cancelation is
  // possible as long as the task is active and executing

 private:
  // we should probably have a weak_ptr to the execution context
  std::shared_ptr<ThreadExecutionContext> execution_context_;
};

struct ThreadTaskExecutor {};

// we need to be able to notify the worker thread to shutdown
struct WorkerThreadInfo {
  std::thread thread;
  Future<void> task;
};

// TODO(lamarrr): think about shutting down process of the pipeline with
// execution contexts. be sure to prevent cyclic references
struct DefaultThreadExecutionContext {
  struct TaskQueueState {
    VLK_DEFAULT_CONSTRUCTOR(TaskQueueState)
    STX_DISABLE_COPY(TaskQueueState)
    VLK_DISABLE_MOVE(TaskQueueState)

    std::queue<PackagedTask> task_queue;
    std::atomic<LockStatus> lock_status = LockStatus::Unlocked;
  };

  struct TaskQueue {
    CacheLineAligned<TaskQueueState> state;

    // producer
    bool try_push(PackagedTask&& task) {
      return impl::try_acquire_then(
          state.value.lock_status,
          [&task, this] {
            state.value.task_queue.push(std::move(task));
            return true;
          },
          [] { return false; });
    }

    // consumer - worker threads, locking and unlocking the task queue mutex
    // should be relatively fast and should only be used for push and try_pop.
    // the worker threads must not hold on to them while executing the tasks. we
    // also want to be able to send cancelation requests to the worker threads
    auto try_pop() {
      return impl::try_acquire_then(
          state.value.lock_status,
          [this]() -> stx::Option<PackagedTask> {
            PackagedTask task = std::move(state.value.task_queue.front());
            state.value.task_queue.pop();
            return stx::Some(std::move(task));
          },
          []() -> stx::Option<PackagedTask> { return stx::None; });
    }
  };

  // create threads
  //
  // request cancelation, mark all pending tasks as canceled, this would mean
  // packaged task would need cancelation tokens for each of the tasks
  //
  // join them
  //
  //
  // handling more submitted tasks?
  //
  std::weak_ptr<ThreadTaskExecutor> executor;

  // we don't need mutex???? for a scheduler that is
  // multithreaded trace context

  std::vector<WorkerThreadInfo> worker_threads_;

  TaskQueue task_queue_;
  std::mutex task_queue_mutex_;

  void shutdown_worker_threads() {
    for (WorkerThreadInfo& worker_thread : worker_threads_) {
      worker_thread.task.request_cancel();
      block_on(worker_thread.task);
    }
  }

  void launch_worker_threads() {
    uint32_t num_threads = std::thread::hardware_concurrency();
  }

  /*
    void worker_thread_task(uint8_t execution_trace_context,
                            CancelationProxy & cancelation_proxy,
                            ThreadInfo info) {
      uint64_t taskless_iterations = 0;
      // sleeping procedure if no task available
      while (!cancelation_proxy.try_acknowledge_cancel()) {
        PackagedTask task;
        {
          task_queue_mutex_.lock();
          if (true) {
            task_queue_mutex_.unlock();
            // TODO(lamarrr): need to sleep and release lock here
            // exponential sleep

            continue;
          }
          task = std::move(task_queue_.front());
          task_queue_.pop();
        }
      }
    }
    */
};
}  // namespace vlk



  /*
    RequestedCancelState fetch_user_requested_cancel_state() const {
      return state->user___fetch_requested_cancel_state();
    }

    RequestedSuspendState fetch_user_requested_suspend_state() const {
      return state->user___fetch_requested_suspend_state();
    }

    RequestedCancelState fetch_executor_requested_cancel_state() const {
      return state->executor___fetch_requested_cancel_state();
    }

    RequestedSuspendState fetch_executor_requested_suspend_state() const {
      return state->executor___fetch_requested_suspend_state();
    }
    */


  /*
    RequestedCancelState executor___fetch_requested_cancel_state() const {
      return executor_requested_cancel_state.load(std::memory_order_relaxed);
    }

    RequestedSuspendState executor___fetch_requested_suspend_state() const {
      return executor_requested_suspend_state.load(std::memory_order_relaxed);
    }
    */

     /*
    RequestedCancelState user___fetch_requested_cancel_state() const {
      return user_requested_cancel_state.load(std::memory_order_relaxed);
    }

    RequestedSuspendState user___fetch_requested_suspend_state() const {
      return user_requested_suspend_state.load(std::memory_order_relaxed);
    }
  */
