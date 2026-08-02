#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include "net/minecraft/util/concurrent/Lifecycle.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::Lifecycle;
TEST(LifecycleTest, ShutdownRunsUnblockStopThenJoin) {
 Lifecycle lifecycle;
 std::vector<std::string> calls;
 std::mutex mutex;
 const auto record = [&](const std::string& step) {
  const std::lock_guard lock(mutex);
  calls.push_back(step);
 };
 lifecycle.registerOwner("a", Lifecycle::Owner{
                                  [&] { record("unblock-a"); },
                                  [&] { record("stop-a"); },
                                  [&](std::chrono::steady_clock::time_point) {
                                   record("join-a");
                                   return true;
                                  },
                                  std::chrono::seconds(5),
                              });
 lifecycle.registerOwner("b", Lifecycle::Owner{
                                  [&] { record("unblock-b"); },
                                  [&] { record("stop-b"); },
                                  [&](std::chrono::steady_clock::time_point) {
                                   record("join-b");
                                   return true;
                                  },
                                  std::chrono::seconds(5),
                              });
 lifecycle.shutdown();
 const std::lock_guard lock(mutex);
 ASSERT_EQ(calls.size(), 6U);
 EXPECT_EQ(calls[0], "unblock-a");
 EXPECT_EQ(calls[1], "unblock-b");
 EXPECT_EQ(calls[2], "stop-a");
 EXPECT_EQ(calls[3], "stop-b");
 EXPECT_EQ(calls[4], "join-a");
 EXPECT_EQ(calls[5], "join-b");
}
TEST(LifecycleTest, JoinReceivesDeadlineAndCanReportTimeout) {
 Lifecycle lifecycle;
 std::atomic<bool> unblocked{false};
 std::atomic<bool> stopped{false};
 std::chrono::steady_clock::time_point suppliedDeadline{};
 lifecycle.registerOwner("leak", Lifecycle::Owner{
                                     [&] { unblocked.store(true, std::memory_order_release); },
                                     [&] { stopped.store(true, std::memory_order_release); },
                                     [&](std::chrono::steady_clock::time_point deadline) {
                                      suppliedDeadline = deadline;
                                      return false;
                                     },
                                     std::chrono::milliseconds(50),
                                 });
 const auto start = std::chrono::steady_clock::now();
 lifecycle.shutdown();
 const auto elapsed =
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
 EXPECT_TRUE(unblocked.load(std::memory_order_acquire));
 EXPECT_TRUE(stopped.load(std::memory_order_acquire));
 EXPECT_GT(suppliedDeadline, start);
 EXPECT_LT(elapsed.count(), 2000);
}
TEST(LifecycleTest, RegisterCanBeQueried) {
 Lifecycle lifecycle;
 EXPECT_EQ(lifecycle.ownerCount(), 0U);
 lifecycle.registerOwner(
     "x", Lifecycle::Owner{.unblock = [] {},
                           .stop = [] {},
                           .join = [](std::chrono::steady_clock::time_point) { return true; }});
 EXPECT_EQ(lifecycle.ownerCount(), 1U);
 lifecycle.shutdown();
}
} // namespace
} // namespace net::minecraft::test
