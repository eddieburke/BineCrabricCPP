#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include "net/minecraft/util/concurrent/Channel.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::concurrent::Channel;
TEST(ChannelTest, TryPushAtCapacityReturnsFalseWithoutBlocking) {
 Channel<int> channel(2);
 EXPECT_TRUE(channel.tryPush(1));
 EXPECT_TRUE(channel.tryPush(2));
 EXPECT_FALSE(channel.tryPush(3));
 int out = 0;
 EXPECT_TRUE(channel.tryPop(out));
 EXPECT_EQ(out, 1);
 EXPECT_TRUE(channel.tryPop(out));
 EXPECT_EQ(out, 2);
 EXPECT_FALSE(channel.tryPop(out));
}
TEST(ChannelTest, BlockingPushWaitsForSpace) {
 Channel<int> channel(1);
 EXPECT_TRUE(channel.tryPush(1));
 std::atomic<bool> pushed{false};
 std::thread producer([&] {
  channel.push(2);
  pushed.store(true, std::memory_order_release);
 });
 std::this_thread::sleep_for(std::chrono::milliseconds(50));
 EXPECT_FALSE(pushed.load(std::memory_order_acquire));
 int out = 0;
 EXPECT_TRUE(channel.tryPop(out));
 EXPECT_EQ(out, 1);
 producer.join();
 EXPECT_TRUE(pushed.load(std::memory_order_acquire));
 EXPECT_TRUE(channel.tryPop(out));
 EXPECT_EQ(out, 2);
}
TEST(ChannelTest, ResetWakesBlockedProducers) {
 Channel<int> channel(1);
 EXPECT_TRUE(channel.tryPush(1));
 std::atomic<bool> pushed{false};
 std::thread producer([&] {
  pushed.store(channel.push(2), std::memory_order_release);
 });
 std::this_thread::sleep_for(std::chrono::milliseconds(50));
 EXPECT_FALSE(pushed.load(std::memory_order_acquire));
 channel.reset();
 producer.join();
 EXPECT_TRUE(pushed.load(std::memory_order_acquire));
}
TEST(ChannelTest, StopMakesPushAndPopReturnFalse) {
 Channel<int> channel(1);
 EXPECT_TRUE(channel.tryPush(1));
 channel.request_stop();
 EXPECT_TRUE(channel.stop_requested());
 EXPECT_FALSE(channel.tryPush(2));
 int out = 0;
 EXPECT_FALSE(channel.tryPop(out));
}
} // namespace
} // namespace net::minecraft::test
