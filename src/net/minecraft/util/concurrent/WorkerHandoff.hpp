#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::util::concurrent {
// Worker pool that hands finished jobs back to the main thread via drainCompleted().
// Worker lambdas run off-thread; all other methods are main-thread only.
template <typename Job>
class WorkerHandoff {
public:
  explicit WorkerHandoff(unsigned threadCount = WorkerPool::recommendedThreadCount(2, 2)) : pool_(threadCount) {
  }
  ~WorkerHandoff() {
    cancelAll();
  }
  WorkerHandoff(const WorkerHandoff&) = delete;
  WorkerHandoff& operator=(const WorkerHandoff&) = delete;
  template <typename Fn>
  void enqueue(std::shared_ptr<Job> job, Fn&& work, int priority = 0) {
    pool_.submit(
        [this, job = std::move(job), work = std::forward<Fn>(work)]() mutable {
          work(*job);
          const std::lock_guard lock(mutex_);
          completed_.push_back(std::move(job));
        },
        priority);
  }
  [[nodiscard]] std::vector<std::shared_ptr<Job>> drainCompleted() {
    const std::lock_guard lock(mutex_);
    return std::exchange(completed_, {});
  }
  void cancelAll() {
    pool_.cancelPending();
    pool_.drain();
    const std::lock_guard lock(mutex_);
    completed_.clear();
  }
  [[nodiscard]] bool idle() const {
    if(pool_.pendingCount() != 0) {
      return false;
    }
    const std::lock_guard lock(mutex_);
    return completed_.empty();
  }
  [[nodiscard]] std::size_t pendingJobs() const {
    return pool_.pendingCount();
  }
  [[nodiscard]] unsigned workerCount() const noexcept {
    return pool_.threadCount();
  }

private:
  WorkerPool pool_;
  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<Job>> completed_;
};
} // namespace net::minecraft::util::concurrent
