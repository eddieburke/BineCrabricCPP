#include <gtest/gtest.h>
#include <chrono>
#include "net/minecraft/client/debug/RenderProfiler.hpp"
namespace net::minecraft::test {
namespace {
void spinFor(std::chrono::microseconds duration) {
 const auto end = std::chrono::steady_clock::now() + duration;
 while(std::chrono::steady_clock::now() < end) {
 }
}
} // namespace
TEST(RenderProfilerTest, FrameMetricsResetAndAccumulateWithoutAllocation) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 profiler.beginFrame();
 profiler.record(client::debug::RenderMetric::EntityTraversals, 7);
 profiler.record(client::debug::RenderMetric::DrawCalls, 3);
 profiler.endFrame();
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::EntityTraversals), 7U);
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::DrawCalls), 3U);
 profiler.beginFrame();
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::EntityTraversals), 0U);
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::DrawCalls), 0U);
 profiler.endFrame();
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, FrameWallTimeIsMeasuredAcrossFrames) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 for(int frame = 0; frame < 64; ++frame) {
  profiler.beginFrame();
  profiler.record(client::debug::RenderMetric::DrawCalls, 1);
  profiler.endFrame();
 }
 EXPECT_GT(profiler.frameAverageNs(), 0.0);
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, FirstFrameUsesTheMeasuredValueWithoutRampUp) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 profiler.beginFrame();
 spinFor(std::chrono::microseconds(500));
 profiler.endFrame();
 EXPECT_GE(profiler.frameAverageNs(), 400000.0);
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, NamedScopesAccumulateWithinAFrame) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 profiler.beginFrame();
 {
  client::debug::RenderProfileScope scope("pipeline", "composite");
  spinFor(std::chrono::microseconds(200));
 }
 {
  client::debug::RenderProfileScope scope("pipeline", "composite");
  spinFor(std::chrono::microseconds(200));
 }
 profiler.endFrame();
 EXPECT_GE(profiler.cpuSpanAverageNs("pipeline/composite"), 300000.0);
 const auto snapshot = profiler.snapshot();
 EXPECT_EQ(snapshot.cpuSpanProbes, 2U);
 profiler.setEnabled(false);
}
} // namespace net::minecraft::test
