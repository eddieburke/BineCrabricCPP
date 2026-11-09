#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>
#define private public
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
#undef private
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
namespace net::minecraft::test {
namespace {
struct TrackedJob {
 std::thread::id* destroyedOn = nullptr;
 ~TrackedJob() {
  if(destroyedOn != nullptr) *destroyedOn = std::this_thread::get_id();
 }
};
} // namespace
TEST(WorkerHandoff, FinishedJobDiesWithTheLastOwnerNotWithTheWorkerTask) {
 util::concurrent::WorkerPool pool(1);
 std::thread::id destroyedOn{};
 std::atomic<bool> started{false};
 std::atomic<bool> release{false};
 {
  util::concurrent::WorkerHandoff<TrackedJob> handoff(pool);
  auto job = std::make_shared<TrackedJob>();
  job->destroyedOn = &destroyedOn;
  handoff.enqueue(std::move(job), [&started, &release](TrackedJob&) {
   started.store(true, std::memory_order_release);
   while(!release.load(std::memory_order_acquire)) std::this_thread::yield();
  });
  while(!started.load(std::memory_order_acquire)) std::this_thread::yield();
  {
   const std::unique_lock poolLock(pool.mutex_);
   release.store(true, std::memory_order_release);
   std::vector<std::shared_ptr<TrackedJob>> drained;
   const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
   while(drained.empty() && std::chrono::steady_clock::now() < deadline) {
    drained = handoff.drainCompleted();
    std::this_thread::yield();
   }
   ASSERT_EQ(drained.size(), 1u);
   EXPECT_EQ(drained.front().use_count(), 1);
   drained.clear();
   EXPECT_EQ(destroyedOn, std::this_thread::get_id());
  }
  pool.drain();
 }
 EXPECT_EQ(destroyedOn, std::this_thread::get_id());
}
TEST(WorkerHandoff, CancelledQueuedJobsStillReachTheCompletedList) {
 util::concurrent::WorkerPool pool(1);
 std::atomic<bool> started{false};
 std::atomic<bool> release{false};
 util::concurrent::WorkerHandoff<TrackedJob> handoff(pool);
 handoff.enqueue(std::make_shared<TrackedJob>(), [&started, &release](TrackedJob&) {
  started.store(true, std::memory_order_release);
  while(!release.load(std::memory_order_acquire)) std::this_thread::yield();
 });
 while(!started.load(std::memory_order_acquire)) std::this_thread::yield();
 for(int i = 0; i < 4; ++i) handoff.enqueue(std::make_shared<TrackedJob>(), [](TrackedJob&) {});
 EXPECT_EQ(handoff.pendingJobs(), 5u);
 pool.cancelPending(&handoff);
 EXPECT_EQ(handoff.pendingJobs(), 1u);
 const std::vector<std::shared_ptr<TrackedJob>> cancelled = handoff.drainCompleted();
 EXPECT_EQ(cancelled.size(), 4u);
 for(const auto& job : cancelled) EXPECT_EQ(job.use_count(), 1);
 release.store(true, std::memory_order_release);
 pool.drain();
 EXPECT_EQ(handoff.pendingJobs(), 0u);
}
} // namespace net::minecraft::test
