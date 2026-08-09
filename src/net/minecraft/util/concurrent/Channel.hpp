#pragma once
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::util::concurrent {
// Worker -> main-thread handoff: bounded FIFO, stop-aware. Producers on the
// main thread use tryPush so they never block; worker producers may block on
// push. Workers never consume: the owner drains in submission order.
template <typename T>
class Channel {
 public:
 explicit Channel(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {
 }
 Channel() : Channel(16) {
 }
 bool push(T value) {
  return pushInternal(std::move(value), true);
 }
 bool tryPush(T value) {
  return pushInternal(std::move(value), false);
 }
 bool pop(T& out) {
  return popInternal(out, true);
 }
 bool tryPop(T& out) {
  return popInternal(out, false);
 }
 // Empty the channel in submission order.
 std::vector<T> drain() {
  std::lock_guard lock(mutex_);
  std::vector<T> result;
  result.reserve(queue_.size());
  while(!queue_.empty()) {
   result.push_back(std::move(queue_.front()));
   queue_.pop_front();
  }
  notFull_.notify_all();
  return result;
 }
 // Drop everything and wake blocked producers; clears the stop state too.
 void reset() {
  std::lock_guard lock(mutex_);
  queue_.clear();
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
 [[nodiscard]] std::stop_token get_stop_token() const {
  std::lock_guard lock(mutex_);
  return stopSource_.get_token();
 }
 [[nodiscard]] bool stop_requested() const {
  std::lock_guard lock(mutex_);
  return stopSource_.stop_requested();
 }
 void request_stop() {
  std::lock_guard lock(mutex_);
  stopSource_.request_stop();
  notEmpty_.notify_all();
  notFull_.notify_all();
 }

 private:
 bool pushInternal(T value, bool wait) {
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
  queue_.push_back(std::move(value));
  notEmpty_.notify_one();
  return true;
 }
 bool popInternal(T& out, bool wait) {
  std::unique_lock lock(mutex_);
  for(;;) {
   if(stopSource_.stop_requested()) {
    return false;
   }
   if(!queue_.empty()) {
    out = std::move(queue_.front());
    queue_.pop_front();
    notFull_.notify_one();
    return true;
   }
   if(!wait) {
    return false;
   }
   notEmpty_.wait(lock, [this] { return stopSource_.stop_requested() || !queue_.empty(); });
  }
 }
 mutable std::mutex mutex_;
 std::condition_variable notEmpty_;
 std::condition_variable notFull_;
 std::deque<T> queue_;
 std::size_t capacity_;
 std::stop_source stopSource_;
};
} // namespace net::minecraft::util::concurrent
