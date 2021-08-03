#pragma once

#include <atomic>
#include <cinttypes>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "stx/result.h"
#include "vlk/primitives.h"
#include "vlk/ui/render_context.h"
#include "vlk/utils.h"

namespace vlk {
namespace ui {

namespace impl {

inline std::string format_bytes_unit(uint64_t bytes) {
  static constexpr uint64_t kb_bytes = 1000UL;
  static constexpr uint64_t mb_bytes = kb_bytes * 1000UL;
  static constexpr uint64_t gb_bytes = mb_bytes * 1000UL;
  static constexpr uint64_t tb_bytes = gb_bytes * 1000UL;

  if (bytes >= (tb_bytes / 10)) {
    return fmt::format("{:.2f} TeraBytes",
                       bytes / static_cast<float>(tb_bytes));
  } else if (bytes >= (gb_bytes / 10)) {
    return fmt::format("{:.2f} GigaBytes",
                       bytes / static_cast<float>(gb_bytes));
  } else if (bytes >= (mb_bytes / 10)) {
    return fmt::format("{:.2f} MegaBytes",
                       bytes / static_cast<float>(mb_bytes));
  } else if (bytes >= (kb_bytes / 10)) {
    return fmt::format("{:.2f} KiloBytes",
                       bytes / static_cast<float>(kb_bytes));
  } else {
    return fmt::format("{} Bytes", bytes);
  }
}

}  // namespace impl

struct Asset {
 public:
  Asset() : size_bytes_{0} {}
  virtual ~Asset() {}

  uint64_t size_bytes() const { return size_bytes_; }

 protected:
  void update_size(uint64_t size) { size_bytes_ = size; }

 private:
  uint64_t size_bytes_ = 0;
};

struct AssetLoadArgs {
  AssetLoadArgs() {}
  virtual ~AssetLoadArgs() {}
};

// loaders can be shared across multiple threads and thus share the same memory
// space. therefore, `load()` is made const to prevent modifying the address
// space across threads (data races) to an extent.
struct AssetLoader {
  AssetLoader() {}
  virtual ~AssetLoader() {}

  // must be thread-safe
  virtual std::unique_ptr<Asset> load(AssetLoadArgs const &) const {
    return std::make_unique<Asset>();
  }
};

struct AssetTag {
  explicit AssetTag(std::string identifier_string) {
    auto shared_handle = std::shared_ptr<std::string>{
        new std::string{std::move(identifier_string)}};
    handle = shared_handle;
  }

  static AssetTag from_static(std::string_view ref) {
    AssetTag identifier;
    identifier.ref = ref;
    identifier.handle = nullptr;
    return identifier;
  }

  static AssetTag from_shared(std::shared_ptr<void const> resource_handle,
                              std::string_view ref) {
    AssetTag identifier;
    identifier.ref = ref;
    identifier.handle = resource_handle;
    return identifier;
  }

  std::string_view as_string_view() const { return ref; }

  bool operator==(AssetTag const &other) const {
    return as_string_view() == other.as_string_view();
  }

  bool operator<(AssetTag const &other) const {
    return as_string_view() < other.as_string_view();
  }

  bool operator>(AssetTag const &other) const {
    return as_string_view() > other.as_string_view();
  }

  bool operator<=(AssetTag const &other) const {
    return as_string_view() <= other.as_string_view();
  }

  bool operator>=(AssetTag const &other) const {
    return as_string_view() >= other.as_string_view();
  }

 private:
  STX_DEFAULT_CONSTRUCTOR(AssetTag)

  std::string_view ref;
  std::shared_ptr<void const> handle;
};

// Asset Manager Requirements
//
// - we want to be able to load by tag, tags must be unique
// - we want to be able to view usage, drop, reload and hit statistics
// - =*= we want to be able to drop the items when not in use or whenever we
// want or feel like
// - =*= we want persistence of the assets in certain cases, i.e. icons that are
// certain to be used in a lot of places and are cheap to have in memory
// - we want to provide asynchronous data loading without blocking the main
// thread.
// - we want to be able to relay the status of the loaded assets
// - guarantee to not discard assets whilst in use
// - consistency of managed objects across ticks. i.e. if widget A tries to get
// asset K in tick 0, the result it gets must be equal to the result widget B
// would get in tick 0

enum class AssetState : uint8_t { Loading, Loaded, Unloaded };

enum class AssetError : uint8_t { TagExists, InvalidTag, IsLoading };

struct AssetManager {
 public:
  AssetManager()
      : data_{},
        submission_queue_{},
        submission_queue_mutex_{},
        completion_queue_{},
        completion_queue_mutex_{},
        cancelation_token_{std::make_shared<AtomicToken>(Token::Running)},
        worker_thread_{worker_thread_task,
                       std::ref(submission_queue_),
                       std::ref(submission_queue_mutex_),
                       std::ref(completion_queue_),
                       std::ref(completion_queue_mutex_),
                       cancelation_token_} {}

  /// `requires_persistence`: some data assets must just persist. i.e. icons and
  /// frequently used data. internet-loaded data file-loaded data should not
  /// necessarily persist
  ///
  /// non-persistent ones will be discarded/unloaded after a period of time
  ///
  ///
  auto add(AssetTag tag, std::unique_ptr<AssetLoadArgs const> &&load_args,
           std::shared_ptr<AssetLoader const> &&loader)
      -> stx::Result<stx::NoneType, AssetError> {
    VLK_ENSURE(load_args != nullptr);
    VLK_ENSURE(loader != nullptr);

    // references are not invalidated
    auto const [iterator, was_inserted] = data_.try_emplace(
        std::move(tag),
        AssetData{std::move(load_args), std::move(loader), stx::None,
                  AssetState::Unloaded, Ticks{}, false});

    if (!was_inserted) {
      return stx::Err(AssetError::TagExists);
    }

    return stx::Ok<stx::NoneType>({});
  }

  // if asset has an entry and it has been discarded, we need to now create make
  // a reload and then return that the asset is being loaded back
  auto get(AssetTag tag)
      -> stx::Result<std::shared_ptr<Asset const>, AssetError> {
    auto const pos = data_.find(tag);
    if (pos == data_.end()) return stx::Err(AssetError::InvalidTag);

    switch (pos->second.state) {
      case AssetState::Loaded:
        pos->second.just_accessed = true;
        return stx::Ok(pos->second.asset.clone().unwrap());
      case AssetState::Loading:
        return stx::Err(AssetError::IsLoading);
      case AssetState::Unloaded:
        pos->second.state = AssetState::Loading;
        // this means data must not be removed from the data map whilst
        // reference is in the submission queue since we are holding a reference
        // to it
        submit_task(SubmissionData{pos->first, pos->second.loader.get(),
                                   pos->second.load_args.get()});

        VLK_LOG("Submitted asset `{}` to worker thread for reloading",
                pos->first.as_string_view());

        return stx::Err(AssetError::IsLoading);

      default:
        VLK_PANIC("Unimplemented");
    }
  }

  void tick(std::chrono::nanoseconds) {
    bool size_changed = false;
    {
      std::lock_guard guard{completion_queue_mutex_};

      while (!completion_queue_.empty()) {
        CompletionData cmpl_data = std::move(completion_queue_.front());
        completion_queue_.pop();
        auto const iter = data_.find(cmpl_data.tag);
        VLK_ENSURE(iter != data_.end());
        iter->second.state = AssetState::Loaded;
        iter->second.asset = stx::Some(std::shared_ptr<Asset const>{
            std::shared_ptr<Asset>{std::move(cmpl_data.asset)}});

        VLK_LOG(
            "Loaded asset with tag `{}` of size: {}",
            iter->first.as_string_view(),
            impl::format_bytes_unit(iter->second.asset.value()->size_bytes()));

        size_changed = true;
      }
    }

    for (auto &[tag, entry] : data_) {
      if (entry.just_accessed) {
        entry.stale_ticks.reset();
      } else if (entry.state == AssetState::Loaded) {
        entry.stale_ticks++;
      }

      entry.just_accessed = false;

      if (entry.stale_ticks > max_stale_ticks_ &&
          entry.state == AssetState::Loaded &&
          entry.asset.value().use_count() == 1) {
        VLK_LOG(
            "Asset with tag `{}` and size {} has been stale and not in "
            "use "
            "for {} ticks. "
            "Asset will be "
            "discarded",
            tag.as_string_view(),
            impl::format_bytes_unit(entry.asset.value()->size_bytes()),
            entry.stale_ticks.count());
        entry.asset = stx::None;
        entry.state = AssetState::Unloaded;
        entry.stale_ticks.reset();

        size_changed = true;
      }
    }

    if (size_changed) {
      uint64_t total_size = 0;
      for (auto &[tag, entry] : data_) {
        if (entry.state == AssetState::Loaded) {
          total_size += entry.asset.value()->size_bytes();
        }
      }
      VLK_LOG("Present total assets size: {}",
              impl::format_bytes_unit(total_size));
    }
  }

  ~AssetManager() { shutdown_worker_thread(); }

 private:
  enum class Token : uint8_t { Running, Cancel, Exited };

  using AtomicToken = std::atomic<Token>;

  using CancelationToken = std::shared_ptr<AtomicToken>;

  // NOTE: only the pointer value of load_args and loader are shared with the
  // worker threads. this prevents data races even along struct members.
  struct AssetData {
    std::unique_ptr<AssetLoadArgs const> load_args;
    std::shared_ptr<AssetLoader const> loader;
    stx::Option<std::shared_ptr<Asset const>> asset;
    AssetState state = AssetState::Loading;
    Ticks stale_ticks = Ticks{0};
    bool just_accessed = false;
  };

  struct SubmissionData {
    AssetTag tag;
    AssetLoader const *loader = nullptr;
    AssetLoadArgs const *load_args = nullptr;
  };

  struct CompletionData {
    AssetTag tag;
    std::unique_ptr<Asset> asset;
  };

  void submit_task(SubmissionData &&subm_data) {
    std::lock_guard guard{submission_queue_mutex_};
    submission_queue_.push(std::move(subm_data));
  }

  static void backoff_spin_delay(uint64_t iteration) {
    if (iteration < 64) {
      // immediate spinning
    } else if (iteration < 128) {
      std::this_thread::yield();
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(125));
    }
  }

  // the worker threads only read the submission data and not modify them
  //
  //
  static void worker_thread_task(std::queue<SubmissionData> &submission_queue,
                                 std::mutex &submission_queue_mutex,
                                 std::queue<CompletionData> &completion_queue,
                                 std::mutex &completion_queue_mutex,
                                 CancelationToken cancelation_token) {
    do {
      bool gotten_task = false;

      // it doesn't make sense to lock the whole queue at once and then start
      // executing the task because the asset load operation could be
      // time-consuming. we thus lock and unlock immediately after popping a
      // task from the queue so that more tasks can be added while we are
      // processing one.

      for (uint64_t i = 0;
           !gotten_task && cancelation_token->load() != Token::Cancel; i++) {
        submission_queue_mutex.lock();

        if (submission_queue.size() != 0) {
          SubmissionData task = std::move(submission_queue.front());
          submission_queue.pop();
          submission_queue_mutex.unlock();
          gotten_task = true;

          CompletionData cmpl_data{std::move(task.tag),
                                   task.loader->load(*task.load_args)};

          {
            std::lock_guard guard{completion_queue_mutex};
            completion_queue.push(std::move(cmpl_data));
          }
        } else {
          submission_queue_mutex.unlock();
          backoff_spin_delay(i);
        }
      }
    } while (cancelation_token->load() != Token::Cancel);

    VLK_LOG("Asset manager worker thread exiting...");
    cancelation_token->store(Token::Exited);
  }

  void shutdown_worker_thread() {
    cancelation_token_->store(Token::Cancel);
    for (uint64_t i = 0; cancelation_token_->load() != Token::Exited; i++) {
      backoff_spin_delay(i);
    }

    worker_thread_.join();
    VLK_LOG("Asset manager worker thread shut down");
  }

  std::map<AssetTag, AssetData, std::less<>> data_;

  std::queue<SubmissionData> submission_queue_;
  std::mutex submission_queue_mutex_;

  std::queue<CompletionData> completion_queue_;
  std::mutex completion_queue_mutex_;

  CancelationToken cancelation_token_;

  std::thread worker_thread_;

  Ticks max_stale_ticks_ = Ticks{100};
};

}  // namespace ui
}  // namespace vlk
