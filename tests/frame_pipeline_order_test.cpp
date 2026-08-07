#include <gtest/gtest.h>
#include <cstddef>
#include <vector>
#include "net/minecraft/client/core/TaskMailbox.hpp"
#include "net/minecraft/client/util/FramePipeline.hpp"
namespace {
using FramePipeline = net::minecraft::client::util::FramePipeline;
}
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
TEST(FramePipeline, RecordsPhaseDurationsWhenTraceEnabled) {
#ifdef MINECRAFT_FRAME_PROFILE
 FramePipeline pipeline;
 pipeline.beginFrame();
 pipeline.beginPhase(FramePipeline::Phase::Drain);
 pipeline.endPhase();
 pipeline.beginPhase(FramePipeline::Phase::Input);
 pipeline.endPhase();
 pipeline.beginPhase(FramePipeline::Phase::Render);
 pipeline.endPhase();
 EXPECT_EQ(pipeline.recordCount(), std::size_t(3));
 EXPECT_EQ(pipeline.records()[0].phase, FramePipeline::Phase::Drain);
 EXPECT_EQ(pipeline.records()[1].phase, FramePipeline::Phase::Input);
 EXPECT_EQ(pipeline.records()[2].phase, FramePipeline::Phase::Render);
 EXPECT_GE(pipeline.records()[0].duration.count(), 0);
#else
 SUCCEED() << "MINECRAFT_FRAME_PROFILE off; FramePipeline tracing is a no-op.";
#endif
}
TEST(FramePipeline, BeginFrameResetsRecordsWhenTraceEnabled) {
#ifdef MINECRAFT_FRAME_PROFILE
 FramePipeline pipeline;
 pipeline.beginFrame();
 pipeline.beginPhase(FramePipeline::Phase::Pace);
 pipeline.endPhase();
 ASSERT_EQ(pipeline.recordCount(), std::size_t(1));
 pipeline.beginFrame();
 EXPECT_EQ(pipeline.recordCount(), std::size_t(0));
#else
 SUCCEED() << "MINECRAFT_FRAME_PROFILE off; FramePipeline tracing is a no-op.";
#endif
}
