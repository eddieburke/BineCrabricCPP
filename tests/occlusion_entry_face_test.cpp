#include <algorithm>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
namespace net::minecraft::test {
TEST(OcclusionEntryFace, LaterEntryCanExposeAUniqueExit) {
 using client::render::chunk::ChunkBuilder;
 using client::render::world::OcclusionQueueEntry;
 std::vector<block::entity::BlockEntity*> blockEntities;
 const auto makeSection = [&blockEntities](int x, int z) {
  auto section = std::make_unique<ChunkBuilder>(nullptr, blockEntities, x * 16, 0, z * 16);
  section->built = true;
  section->renderLayerEmpty = {false, true, true, true};
  return section;
 };
 auto start = makeSection(0, 0);
 auto first = makeSection(1, 0);
 auto second = makeSection(0, 1);
 auto junction = makeSection(1, 1);
 auto destination = makeSection(2, 1);
 start->neighbors[1] = first.get();
 first->neighbors[0] = start.get();
 start->neighbors[5] = second.get();
 second->neighbors[4] = start.get();
 first->neighbors[5] = junction.get();
 junction->neighbors[4] = first.get();
 second->neighbors[1] = junction.get();
 junction->neighbors[0] = second.get();
 junction->neighbors[1] = destination.get();
 destination->neighbors[0] = junction.get();
 junction->occlusion.visBits = (1ULL << 1) | (1ULL << 6);
 client::render::Frustum culler;
 std::vector<OcclusionQueueEntry> queue;
 std::vector<ChunkBuilder*> visible;
 client::render::world::cullByOcclusionWalk(start.get(), culler, 1, queue, visible);
 EXPECT_NE(std::find(visible.begin(), visible.end(), destination.get()), visible.end());
 EXPECT_EQ(std::count(visible.begin(), visible.end(), destination.get()), 1);
}
}
