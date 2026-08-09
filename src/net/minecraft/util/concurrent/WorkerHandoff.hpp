#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::util::concurrent {
template <typename Job>
class WorkerHandoff {
 public:
  explicit WorkerHandoff(WorkerPool& pool)
      : pool_(pool) {
  }
  ~WorkerHandoff() {
   cancelAll();
   while(inFlight_.load(std::memory_order::acquire) != 0) {
    std::vector<std::shared_ptr<Job>> remaining = drainCompleted();
    remaining.clear();
    std::this_thread::yield();
   }
   std::vector<std::shared_ptr<Job>> remaining = drainCompleted();
   remaining.clear();
  }
  WorkerHandoff(const WorkerHandoff&) = delete;
  WorkerHandoff& operator=(const WorkerHandoff&) = delete;
  template <typename Fn>
  void enqueue(std::shared_ptr<Job> job, Fn&& work, int priority = 0) {
   const std::uint64_t epoch = epoch_.load(std::memory_order::acquire);
   inFlight_.fetch_add(1, std::memory_order::release);
   pool_.submit(
       [this, job = std::move(job), work = std::forward<Fn>(work), epoch]() mutable {
        if(epoch_.load(std::memory_order::acquire) == epoch) {
         work(*job);
        }
        {
         const std::lock_guard lock(completedMutex_);
         completed_.push_back(std::move(job));
        }
        inFlight_.fetch_sub(1, std::memory_order::release);
       },
       priority);
  }
  [[nodiscard]] std::vector<std::shared_ptr<Job>> drainCompleted() {
   const std::lock_guard lock(completedMutex_);
   std::vector<std::shared_ptr<Job>> drained;
   drained.swap(completed_);
   return drained;
  }
  void cancelAll() {
   epoch_.fetch_add(1, std::memory_order::acq_rel);
   std::vector<std::shared_ptr<Job>> dropped = drainCompleted();
   dropped.clear();
  }
  [[nodiscard]] bool idle() const {
   const std::lock_guard lock(completedMutex_);
   return inFlight_.load(std::memory_order::acquire) == 0 && completed_.empty();
  }
  [[nodiscard]] std::size_t pendingJobs() const {
   return inFlight_.load(std::memory_order::acquire);
  }
  [[nodiscard]] unsigned workerCount() const noexcept {
   return pool_.threadCount();
  }

 private:
  WorkerPool& pool_;
  mutable std::mutex completedMutex_;
  std::vector<std::shared_ptr<Job>> completed_;
  std::atomic<std::uint64_t> epoch_{0};
  std::atomic<std::size_t> inFlight_{0};
};
} // namespace net::minecraft::util::concurrent
