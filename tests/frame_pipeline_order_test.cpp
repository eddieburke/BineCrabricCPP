#include <gtest/gtest.h>
#include <cstddef>
#include <vector>
#include "net/minecraft/client/core/TaskMailbox.hpp"
#include "net/minecraft/client/util/FramePipeline.hpp"
#include "net/minecraft/client/util/FrameProfiler.hpp"
namespace {
using FramePipeline = net::minecraft::client::util::FramePipeline;
using FrameProfiler = net::minecraft::client::util::FrameProfiler;
} // namespace
TEST(FramePipeline, EnumeratesPhasesInDrainToDiagnosticsOrder) {
 const FramePipeline::Phase expected[] = {FramePipeline::Phase::Drain, FramePipeline::Phase::Input,
                                          FramePipeline::Phase::Ticks, FramePipeline::Phase::Render,
                                          FramePipeline::Phase::Pace, FramePipeline::Phase::Diagnostics};
 ASSERT_EQ(FramePipeline::count(), std::size_t(6));
 for(std::size_t i = 0; i < FramePipeline::count(); ++i) {
  EXPECT_EQ(FramePipeline::phaseAt(i), expected[i]);
 }
}
TEST(FramePipeline, RunVisitsEveryPhaseWithoutSideEffects) {
 FramePipeline pipeline;
 pipeline.run();
}
TEST(TaskMailbox, PreservesFifoOrderWithinPriority) {
 net::minecraft::client::core::TaskMailbox mailbox;
 std::vector<int> order;
 for(int i = 0; i < 5; ++i) {
  mailbox.pushTick([&order, i] { order.push_back(i); });
 }
 ASSERT_EQ(mailbox.size(), std::size_t(5));
 EXPECT_EQ(mailbox.drainTick(), std::size_t(5));
 EXPECT_EQ(order, (std::vector<int>{0, 1, 2, 3, 4}));
}
TEST(TaskMailbox, PrioritiesDrainIndependently) {
 net::minecraft::client::core::TaskMailbox mailbox;
 std::vector<int> order;
 mailbox.pushUrgent([&order] { order.push_back(1); });
 mailbox.pushRender([&order] { order.push_back(2); });
 mailbox.pushTick([&order] { order.push_back(3); });
 ASSERT_EQ(mailbox.size(), std::size_t(3));
 EXPECT_EQ(mailbox.drainUrgent(), std::size_t(1));
 EXPECT_EQ(order, (std::vector<int>{1}));
 EXPECT_EQ(mailbox.drainRender(), std::size_t(1));
 EXPECT_EQ(order, (std::vector<int>{1, 2}));
 EXPECT_EQ(mailbox.drainTick(), std::size_t(1));
 EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
 EXPECT_EQ(mailbox.size(), std::size_t(0));
}
TEST(TaskMailbox, EmptyDrainReturnsZero) {
 net::minecraft::client::core::TaskMailbox mailbox;
 EXPECT_EQ(mailbox.size(), std::size_t(0));
 EXPECT_EQ(mailbox.drainUrgent(), std::size_t(0));
 EXPECT_EQ(mailbox.drainTick(), std::size_t(0));
 EXPECT_EQ(mailbox.drainRender(), std::size_t(0));
}
TEST(FrameProfiler, RecordsPhaseDurationsWhenTraceEnabled) {
#ifdef MINECRAFT_FRAME_PROFILE
 FrameProfiler& profiler = FrameProfiler::instance();
 profiler.beginFrame();
 profiler.beginPhase(FramePipeline::Phase::Drain);
 profiler.endPhase();
 profiler.beginPhase(FramePipeline::Phase::Input);
 profiler.endPhase();
 profiler.beginPhase(FramePipeline::Phase::Render);
 profiler.endPhase();
 EXPECT_EQ(profiler.recordCount(), std::size_t(3));
 EXPECT_EQ(profiler.records()[0].phase, FramePipeline::Phase::Drain);
 EXPECT_EQ(profiler.records()[1].phase, FramePipeline::Phase::Input);
 EXPECT_EQ(profiler.records()[2].phase, FramePipeline::Phase::Render);
 EXPECT_GE(profiler.records()[0].duration.count(), 0);
#else
 SUCCEED() << "MINECRAFT_FRAME_PROFILE off; FrameProfiler is a no-op.";
#endif
}
TEST(FrameProfiler, BeginFrameResetsRecordsWhenTraceEnabled) {
#ifdef MINECRAFT_FRAME_PROFILE
 FrameProfiler& profiler = FrameProfiler::instance();
 profiler.beginFrame();
 profiler.beginPhase(FramePipeline::Phase::Pace);
 profiler.endPhase();
 ASSERT_EQ(profiler.recordCount(), std::size_t(1));
 profiler.beginFrame();
 EXPECT_EQ(profiler.recordCount(), std::size_t(0));
#else
 SUCCEED() << "MINECRAFT_FRAME_PROFILE off; FrameProfiler is a no-op.";
#endif
}
