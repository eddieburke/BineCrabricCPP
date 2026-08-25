#include <vector>
#include <gtest/gtest.h>
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#undef private
namespace net::minecraft::test {
TEST(ChunkCompilePriority, VisibleSectionsPrecedeCloserInvisibleSections) {
 using client::render::chunk::ChunkBuilder;
 std::vector<block::entity::BlockEntity*> blockEntities;
 ChunkBuilder invisibleNear(nullptr, blockEntities, 0, 64, 0);
 ChunkBuilder visibleFar(nullptr, blockEntities, 64, 64, 0);
 ChunkBuilder invisibleFar(nullptr, blockEntities, 96, 64, 0);
 ChunkBuilder visibleNear(nullptr, blockEntities, 16, 64, 0);
 constexpr int stamp = 7;
 visibleFar.visibleStamp = stamp;
 visibleNear.visibleStamp = stamp;
 std::vector<ChunkBuilder*> candidates{&invisibleNear, &visibleFar, &invisibleFar, &visibleNear};
 client::render::ChunkCompilePipeline::prioritizeCaptureCandidates(candidates, stamp, 0, 4, 0, 1);
 EXPECT_EQ(candidates[0], &visibleNear);
 EXPECT_EQ(candidates[1], &visibleFar);
 EXPECT_EQ(candidates[2], &invisibleNear);
 EXPECT_EQ(candidates[3], &invisibleFar);
}
}
