#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
namespace net::minecraft::test {
namespace {
class NotifyCounter : public GameEventListener {
 public:
 explicit NotifyCounter(int* count) : count_(count) {
 }
 void notifyAmbientDarknessChanged() override {
  ++(*count_);
 }

 private:
 int* count_;
};
TEST(AmbientDarknessSim, FullDaySweepCountsDarknessSteps) {
 World world("AmbientSim", 12345ULL);
 int notifies = 0;
 NotifyCounter counter(&notifies);
 world.addEventListener(&counter);
 std::vector<std::uint64_t> transitionTicks;
 int previous = world.ambientDarkness;
 for(std::uint64_t tick = 0; tick <= 24000ULL; ++tick) {
  world.setTime(tick);
  EXPECT_GE(world.ambientDarkness, 0);
  EXPECT_LE(world.ambientDarkness, 11);
  if(world.ambientDarkness != previous) {
   transitionTicks.push_back(tick);
   previous = world.ambientDarkness;
  }
 }
 EXPECT_EQ(notifies, static_cast<int>(transitionTicks.size()));
 EXPECT_GT(transitionTicks.size(), 2U);
 EXPECT_LT(transitionTicks.size(), 40U);
 const int secondDayNotifies = notifies;
 for(std::uint64_t tick = 24001ULL; tick <= 48000ULL; ++tick) {
  world.setTime(tick);
 }
 EXPECT_EQ(notifies - secondDayNotifies, static_cast<int>(transitionTicks.size()));
 world.setTime(48000ULL);
 EXPECT_EQ(notifies - secondDayNotifies, static_cast<int>(transitionTicks.size()));
 std::printf("[AMBIENT_SIM] day sweep: %d darkness steps at ticks:", static_cast<int>(transitionTicks.size()));
 for(const std::uint64_t tick : transitionTicks) {
  std::printf(" %llu", static_cast<unsigned long long>(tick));
 }
 std::printf("\n");
 world.removeEventListener(&counter);
}
class AmbientPacing : public ::testing::Test {
 protected:
 static constexpr int kGrid = 12;
 static constexpr int kSections = kGrid * kGrid * 8;
 void SetUp() override {
  pipeline_ = std::make_unique<client::render::ChunkCompilePipeline>(scene_);
  sections_ = std::make_unique<client::render::ChunkSectionSystem>(scene_);
  pipeline_->setSectionSystem(*sections_);
  sections_->setCompilePipeline(*pipeline_);
  for(int x = 0; x < kGrid; ++x) {
   for(int z = 0; z < kGrid; ++z) {
    client::render::world::SectionColumn column;
    for(int y = 0; y < 8; ++y) {
     auto section = std::make_shared<client::render::chunk::ChunkBuilder>(
         nullptr, scene_.blockEntities, x * 16, y * 16, z * 16);
     section->hasSkyLight = true;
     section->built = true;
     column[static_cast<std::size_t>(y)] = section;
    }
    sections_->columns_[client::render::world::ColumnPos{x, z}] = std::move(column);
   }
  }
 }
 int dirtySections() const {
  int dirty = 0;
  for(const auto& entry : sections_->columns_) {
   for(const auto& section : entry.second) {
    if(section != nullptr && section->dirty) {
     ++dirty;
    }
   }
  }
  return dirty;
 }
 client::render::TerrainScene scene_;
 std::unique_ptr<client::render::ChunkCompilePipeline> pipeline_;
 std::unique_ptr<client::render::ChunkSectionSystem> sections_;
};
TEST_F(AmbientPacing, NotifyQueuesInsteadOfInvalidating) {
 sections_->notifyAmbientDarknessChanged();
 EXPECT_EQ(dirtySections(), 0);
 EXPECT_EQ(sections_->pendingAmbientColumns_.size(), static_cast<std::size_t>(kGrid * kGrid));
}
TEST_F(AmbientPacing, DrainRespectsSectionBudget) {
 sections_->notifyAmbientDarknessChanged();
 sections_->drainAmbientInvalidations(100);
 const int dirty = dirtySections();
 EXPECT_GE(dirty, 100);
 EXPECT_LE(dirty, 107);
 EXPECT_FALSE(sections_->pendingAmbientColumns_.empty());
 sections_->drainAmbientInvalidations(1000000);
 EXPECT_EQ(dirtySections(), kSections);
 EXPECT_TRUE(sections_->pendingAmbientColumns_.empty());
 EXPECT_TRUE(sections_->pendingAmbientSet_.empty());
}
TEST_F(AmbientPacing, RenotifyCoalescesInsteadOfDoubling) {
 sections_->notifyAmbientDarknessChanged();
 sections_->drainAmbientInvalidations(100);
 sections_->notifyAmbientDarknessChanged();
 sections_->drainAmbientInvalidations(1000000);
 EXPECT_EQ(dirtySections(), kSections);
 for(const auto& entry : sections_->columns_) {
  for(const auto& section : entry.second) {
   ASSERT_NE(section, nullptr);
   EXPECT_EQ(section->version, 1);
  }
 }
}
TEST_F(AmbientPacing, DrainDropsUnloadedColumns) {
 sections_->notifyAmbientDarknessChanged();
 sections_->columns_.erase(client::render::world::ColumnPos{0, 0});
 sections_->drainAmbientInvalidations(1000000);
 EXPECT_EQ(dirtySections(), kSections - 8);
 EXPECT_TRUE(sections_->pendingAmbientColumns_.empty());
}
TEST_F(AmbientPacing, ZeroBudgetDrainsNothing) {
 sections_->notifyAmbientDarknessChanged();
 sections_->drainAmbientInvalidations(0);
 EXPECT_EQ(dirtySections(), 0);
 EXPECT_EQ(sections_->pendingAmbientColumns_.size(), static_cast<std::size_t>(kGrid * kGrid));
}
} // namespace
} // namespace net::minecraft::test
