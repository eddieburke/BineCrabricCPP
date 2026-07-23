#pragma once
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>
namespace net::minecraft::util::concurrent {
// Fixed-size pool of std::jthread workers draining a priority task queue.
// Lower priority value runs first; ties run in submission order.
// Destruction requests stop, wakes all workers, and joins them; queued tasks
// that never started are dropped. Tasks must not throw — wrap fallible work
// and store the error in the task's own result object.
class WorkerPool {
 public:
 explicit WorkerPool(unsigned threadCount) {
  if(threadCount == 0) {
   threadCount = 1;
  }
  workers_.reserve(threadCount);
  for(unsigned i = 0; i < threadCount; ++i) {
   workers_.emplace_back([this](const std::stop_token& stop) { workerLoop(stop); });
  }
 }
 ~WorkerPool() {
  for(auto& worker : workers_) {
   worker.request_stop();
  }
  wake_.notify_all();
  // jthread joins on destruction.
 }
 WorkerPool(const WorkerPool&) = delete;
 WorkerPool& operator=(const WorkerPool&) = delete;
 void submit(std::function<void()> task, int priority = 0) {
  {
   const std::lock_guard lock(mutex_);
   queue_.push(Task{priority, nextSequence_++, std::move(task)});
  }
  wake_.notify_one();
 }
 // Drop all tasks that have not started yet. In-flight tasks finish normally.
 void cancelPending() {
  const std::lock_guard lock(mutex_);
  queue_ = {};
 }
 // Block until the queue is empty and every worker is idle.
 void drain() {
  std::unique_lock lock(mutex_);
  idle_.wait(lock, [this] { return queue_.empty() && activeCount_ == 0; });
 }
 [[nodiscard]] std::size_t pendingCount() const {
  const std::lock_guard lock(mutex_);
  return queue_.size() + static_cast<std::size_t>(activeCount_);
 }
 [[nodiscard]] unsigned threadCount() const noexcept {
  return static_cast<unsigned>(workers_.size());
 }
 [[nodiscard]] static unsigned recommendedThreadCount(unsigned competingPools = 1,
                                                      unsigned reservedThreads = 1,
                                                      unsigned maxThreads = 8) noexcept {
  const unsigned hardwareThreads = std::max(1U, std::thread::hardware_concurrency());
  const unsigned availableThreads = hardwareThreads > reservedThreads ? hardwareThreads - reservedThreads : 1U;
  const unsigned poolCount = std::max(1U, competingPools);
  return std::clamp(availableThreads / poolCount, 1U, std::max(1U, maxThreads));
 }

 private:
 struct Task {
  int priority = 0;
  std::uint64_t sequence = 0;
  std::function<void()> run;
  bool operator<(const Task& other) const noexcept {
   // std::priority_queue is a max-heap; invert so lower priority value pops first.
   if(priority != other.priority) {
    return priority > other.priority;
   }
   return sequence > other.sequence;
  }
 };
 void workerLoop(const std::stop_token& stop) {
  for(;;) {
   Task task;
   {
    std::unique_lock lock(mutex_);
    wake_.wait(lock, [&] { return stop.stop_requested() || !queue_.empty(); });
    if(stop.stop_requested()) {
     return;
    }
    task = std::move(const_cast<Task&>(queue_.top()));
    queue_.pop();
    ++activeCount_;
   }
   try {
    task.run();
   } catch(...) {
   }
   {
    const std::lock_guard lock(mutex_);
    --activeCount_;
    if(queue_.empty() && activeCount_ == 0) {
     idle_.notify_all();
    }
   }
  }
 }
 mutable std::mutex mutex_;
 std::condition_variable wake_;
 std::condition_variable idle_;
 std::priority_queue<Task> queue_;
 std::uint64_t nextSequence_ = 0;
 int activeCount_ = 0;
 std::vector<std::jthread> workers_;
};
} // namespace net::minecraft::util::concurrent
