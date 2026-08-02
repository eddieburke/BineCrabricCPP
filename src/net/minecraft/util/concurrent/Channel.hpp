#pragma once
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::util::concurrent {
// Worker -> main-thread handoff: bounded, prioritized (Urgent first), stop-aware
// and version-stamped for stale-entry dropping. Producers on the main thread use
// tryPush so they never block; worker producers may block on push. A push(T,
// stamp) marks the value with the producer's version; tryPop drops entries older
// than latestStamp() so superseded work never reaches the consumer.
template <typename T>
class Channel {
 public:
  explicit Channel(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {
  }
  Channel() : Channel(16) {
  }
  bool push(T value) {
   return pushInternal(std::move(value), TaskPriority::Normal, 0, true);
  }
  bool push(T value, std::uint64_t stamp) {
   return pushInternal(std::move(value), TaskPriority::Normal, stamp, true);
  }
  bool push(T value, TaskPriority priority) {
   return pushInternal(std::move(value), priority, 0, true);
  }
  bool push(T value, TaskPriority priority, std::uint64_t stamp) {
   return pushInternal(std::move(value), priority, stamp, true);
  }
  bool tryPush(T value) {
   return pushInternal(std::move(value), TaskPriority::Normal, 0, false);
  }
  bool tryPush(T value, std::uint64_t stamp) {
   return pushInternal(std::move(value), TaskPriority::Normal, stamp, false);
  }
  bool tryPush(T value, TaskPriority priority) {
   return pushInternal(std::move(value), priority, 0, false);
  }
  bool tryPush(T value, TaskPriority priority, std::uint64_t stamp) {
   return pushInternal(std::move(value), priority, stamp, false);
  }
  bool pop(T& out) {
   return popInternal(out, true);
  }
  bool tryPop(T& out) {
   return popInternal(out, false);
  }
  // Empty the channel in priority order (highest first, then submission order).
  std::vector<T> drain() {
   std::lock_guard lock(mutex_);
   std::vector<T> result;
   result.reserve(queue_.size());
   while(!queue_.empty()) {
    result.push_back(std::move(takeBestLocked()));
   }
   notFull_.notify_all();
   return result;
  }
  // Drop everything and wake blocked producers; clears the stop state too.
  void reset() {
   std::lock_guard lock(mutex_);
   queue_.clear();
   latestStamp_ = 0;
   stopSource_ = std::stop_source();
   notEmpty_.notify_all();
   notFull_.notify_all();
  }
  void setCapacity(std::size_t capacity) {
   std::lock_guard lock(mutex_);
   capacity_ = std::max<std::size_t>(1, capacity);
   notFull_.notify_all();
  }
  [[nodiscard]] std::size_t size() const {
   std::lock_guard lock(mutex_);
   return queue_.size();
  }
  [[nodiscard]] std::size_t capacity() const {
   std::lock_guard lock(mutex_);
   return capacity_;
  }
  [[nodiscard]] std::uint64_t latestStamp() const noexcept {
   std::lock_guard lock(mutex_);
   return latestStamp_;
  }
  [[nodiscard]] std::stop_token get_stop_token() const noexcept {
   return stopSource_.get_token();
  }
  [[nodiscard]] bool stop_requested() const noexcept {
   return stopSource_.stop_requested();
  }
  void request_stop() {
   stopSource_.request_stop();
   notEmpty_.notify_all();
   notFull_.notify_all();
  }

 private:
  struct Entry {
   Entry(int priorityIn, std::uint64_t sequenceIn, std::uint64_t stampIn, T valueIn)
       : priority(priorityIn), sequence(sequenceIn), stamp(stampIn), value(std::move(valueIn)) {
   }
   int priority = 0;
   std::uint64_t sequence = 0;
   std::uint64_t stamp = 0;
   T value;
  };
  bool pushInternal(T value, TaskPriority priority, std::uint64_t stamp, bool wait) {
   std::unique_lock lock(mutex_);
   if(stopSource_.stop_requested()) {
    return false;
   }
   if(wait) {
    notFull_.wait(lock, [this] { return stopSource_.stop_requested() || queue_.size() < capacity_; });
    if(stopSource_.stop_requested()) {
     return false;
    }
   } else if(queue_.size() >= capacity_) {
    return false;
   }
   latestStamp_ = std::max(latestStamp_, stamp);
   queue_.push_back(Entry{static_cast<int>(priority), nextSequence_++, stamp, std::move(value)});
   notEmpty_.notify_one();
   return true;
  }
  bool popInternal(T& out, bool wait) {
   std::unique_lock lock(mutex_);
   for(;;) {
    if(stopSource_.stop_requested()) {
     return false;
    }
    dropStaleLocked();
    if(!queue_.empty()) {
     out = std::move(takeBestLocked());
     notFull_.notify_one();
     return true;
    }
    if(!wait) {
     return false;
    }
    notEmpty_.wait(lock, [this] { return stopSource_.stop_requested() || !queue_.empty(); });
   }
  }
  void dropStaleLocked() {
   for(auto it = queue_.begin(); it != queue_.end();) {
    if(it->stamp < latestStamp_) {
     it = queue_.erase(it);
    } else {
     ++it;
    }
   }
  }
  // Pops the highest-priority entry (ties by submission order). Caller holds mutex_.
  T takeBestLocked() {
   std::size_t best = 0;
   for(std::size_t i = 1; i < queue_.size(); ++i) {
    if(queue_[i].priority < queue_[best].priority ||
       (queue_[i].priority == queue_[best].priority && queue_[i].sequence < queue_[best].sequence)) {
     best = i;
    }
   }
   T value = std::move(queue_[best].value);
   queue_.erase(queue_.begin() + static_cast<std::ptrdiff_t>(best));
   return value;
  }
  mutable std::mutex mutex_;
  std::condition_variable notEmpty_;
  std::condition_variable notFull_;
  std::deque<Entry> queue_;
  std::uint64_t nextSequence_ = 0;
  std::uint64_t latestStamp_ = 0;
  std::size_t capacity_;
  std::stop_source stopSource_;
};
} // namespace net::minecraft::util::concurrent
