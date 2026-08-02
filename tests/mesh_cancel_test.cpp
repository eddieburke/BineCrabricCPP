#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::WorkerHandoff;
using net::minecraft::util::concurrent::WorkerPool;
// Mirrors the ChunkMeshJob -> ChunkBuilder ownership contract (R3): the last
// shared_ptr must die on the main thread, and ~Job clears the owner's in-flight
// flag so a dropped job can never strand it (HZ-10/HZ-31).
struct TestJob {
 int id = 0;
 std::atomic<bool>* ownerFlag = nullptr;
 ~TestJob() {
  if(ownerFlag != nullptr) {
   ownerFlag->store(false, std::memory_order_release);
  }
 }
};
// cancelAll() must be non-blocking (no pool_.drain()), drop queued work via the
// epoch token, and hand every affected job back so the main thread clears its
// owner's meshJobInFlight-equivalent flag.
TEST(MeshCancel, CancelAllIsNonBlockingAndClearsOwnerFlags) {
 WorkerPool pool(4);
 WorkerHandoff<TestJob> handoff(pool);
 constexpr int kJobCount = 1000;
 std::vector<std::atomic<bool>> flags(static_cast<std::size_t>(kJobCount));
 for(int i = 0; i < kJobCount; ++i) {
  flags[static_cast<std::size_t>(i)].store(true, std::memory_order_release);
  auto job = std::make_shared<TestJob>();
  job->id = i;
  job->ownerFlag = &flags[static_cast<std::size_t>(i)];
  handoff.enqueue(std::move(job), [](TestJob&) {});
 }
 const auto start = std::chrono::steady_clock::now();
 handoff.cancelAll();
 const auto elapsed = std::chrono::steady_clock::now() - start;
 EXPECT_LT(elapsed, std::chrono::milliseconds(50)) << "cancelAll() must not drain the worker pool";
 // cancelAll() drops queued work; workers hand the rest back via the channel.
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(!handoff.idle() && std::chrono::steady_clock::now() < deadline) {
  std::vector<std::shared_ptr<TestJob>> drained = handoff.drainCompleted();
  drained.clear();
  std::this_thread::yield();
 }
 EXPECT_TRUE(handoff.idle());
 for(int i = 0; i < kJobCount; ++i) {
  EXPECT_FALSE(flags[static_cast<std::size_t>(i)].load(std::memory_order_acquire))
      << "job " << i << " owner flag was not cleared after cancelAll";
 }
}
// A single worker completes jobs in submission order; the channel must hand
// them back in the same order (FIFO, no priority scrambling).
TEST(MeshCancel, CompletedJobsDrainInSubmissionOrder) {
 WorkerPool pool(1);
 WorkerHandoff<TestJob> handoff(pool);
 constexpr int kJobCount = 64;
 for(int i = 0; i < kJobCount; ++i) {
  auto job = std::make_shared<TestJob>();
  job->id = i;
  handoff.enqueue(std::move(job), [](TestJob&) {});
 }
 std::vector<int> ids;
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(ids.size() < static_cast<std::size_t>(kJobCount) && std::chrono::steady_clock::now() < deadline) {
  for(std::shared_ptr<TestJob>& job : handoff.drainCompleted()) {
   ids.push_back(job->id);
  }
  std::this_thread::yield();
 }
 ASSERT_EQ(ids.size(), static_cast<std::size_t>(kJobCount));
 for(int i = 0; i < kJobCount; ++i) {
  EXPECT_EQ(ids[static_cast<std::size_t>(i)], i);
 }
}
} // namespace
} // namespace net::minecraft::test
