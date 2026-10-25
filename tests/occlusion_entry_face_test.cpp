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
TEST(OcclusionEntryFace, OpenSectionsEnterTheQueueOnce) {
 using client::render::chunk::ChunkBuilder;
 std::vector<block::entity::BlockEntity*> blockEntities;
 std::vector<std::unique_ptr<ChunkBuilder>> sections;
 constexpr int side = 8;
 for(int z = 0; z < side; ++z) {
  for(int x = 0; x < side; ++x) {
   auto section = std::make_unique<ChunkBuilder>(nullptr, blockEntities, x * 16, 0, z * 16);
   section->built = true;
   section->renderLayerEmpty = {false, true, true, true};
   sections.push_back(std::move(section));
  }
 }
 for(int z = 0; z < side; ++z) {
  for(int x = 0; x < side; ++x) {
   auto& node = *sections[static_cast<std::size_t>(z * side + x)];
   if(x > 0) node.neighbors[0] = sections[static_cast<std::size_t>(z * side + x - 1)].get();
   if(x + 1 < side) node.neighbors[1] = sections[static_cast<std::size_t>(z * side + x + 1)].get();
   if(z > 0) node.neighbors[4] = sections[static_cast<std::size_t>((z - 1) * side + x)].get();
   if(z + 1 < side) node.neighbors[5] = sections[static_cast<std::size_t>((z + 1) * side + x)].get();
  }
 }
 client::render::Frustum culler;
 std::vector<client::render::world::OcclusionQueueEntry> queue;
 std::vector<ChunkBuilder*> visible;
 for(int stamp = 1; stamp <= 3; ++stamp) {
  for(auto& section : sections) {
   section->built = stamp != 3;
   section->occlusion.visBits = stamp == 1 ? (1ULL << 36) - 1 : stamp == 2 ? ~0ULL : 0ULL;
  }
  visible.clear();
  client::render::world::cullByOcclusionWalk(sections.front().get(), culler, stamp, queue, visible);
  EXPECT_EQ(queue.size(), sections.size());
  EXPECT_EQ(visible.size(), sections.size());
  std::sort(visible.begin(), visible.end(), std::less<ChunkBuilder*>{});
  EXPECT_EQ(std::unique(visible.begin(), visible.end()), visible.end());
 }
}
TEST(OcclusionEntryFace, MixedConnectivityMatchesFaceByFaceTraversal) {
 using client::render::chunk::ChunkBuilder;
 using client::render::world::OcclusionQueueEntry;
 std::vector<block::entity::BlockEntity*> blockEntities;
 std::vector<std::unique_ptr<ChunkBuilder>> sections;
 constexpr int side = 9;
 constexpr int dx[] = {-1, 1, 0, 0, 0, 0};
 constexpr int dz[] = {0, 0, 0, 0, -1, 1};
 for(int z = 0; z < side; ++z) {
  for(int x = 0; x < side; ++x) {
   sections.push_back(std::make_unique<ChunkBuilder>(nullptr, blockEntities, x * 16, 0, z * 16));
  }
 }
 for(int z = 0; z < side; ++z) {
  for(int x = 0; x < side; ++x) {
   for(int face : {0, 1, 4, 5}) {
    const int nx = x + dx[face];
    const int nz = z + dz[face];
    if(nx >= 0 && nx < side && nz >= 0 && nz < side) {
     sections[static_cast<std::size_t>(z * side + x)]->neighbors[face] =
         sections[static_cast<std::size_t>(nz * side + nx)].get();
    }
   }
  }
 }
 std::uint64_t random = 12345;
 client::render::Frustum culler;
 std::vector<OcclusionQueueEntry> queue;
 std::vector<ChunkBuilder*> expected;
 std::vector<ChunkBuilder*> actual;
 for(int trial = 0; trial < 100; ++trial) {
  for(auto& node : sections) {
   random = random * 6364136223846793005ULL + 1;
   node->built = (random & 7) != 0;
   node->occlusion.visBits = (random & 3) == 0 ? ~0ULL : random >> 16;
   node->renderLayerEmpty = {(random & 8) == 0, true, true, true};
  }
  const int stamp = trial * 2 + 1;
  auto* start = sections[static_cast<std::size_t>(side * side / 2)].get();
  expected.clear();
  queue.clear();
  (void)start->occlusion.enter(stamp, -1);
  start->visibleStamp = stamp;
  if(!start->hasNoGeometry()) expected.push_back(start);
  queue.push_back({start, -1});
  for(std::size_t head = 0; head < queue.size(); ++head) {
   const auto entry = queue[head];
   for(int face = 0; face < 6; ++face) {
    if(!entry.section->occlusion.claimExit(stamp, entry.entryFace, face, entry.section->built)) continue;
    auto* neighbor = entry.section->neighbors[face];
    if(neighbor == nullptr || !neighbor->occlusion.enter(stamp, face ^ 1)) continue;
    const bool visible = neighbor->visibleIn(stamp);
    if(visible || neighbor->updateFrustum(culler, stamp)) {
     if(!visible && !neighbor->hasNoGeometry()) expected.push_back(neighbor);
     queue.push_back({neighbor, face ^ 1});
    }
   }
  }
  actual.clear();
  client::render::world::cullByOcclusionWalk(start, culler, stamp + 1, queue, actual);
  std::sort(expected.begin(), expected.end(), std::less<ChunkBuilder*>{});
  std::sort(actual.begin(), actual.end(), std::less<ChunkBuilder*>{});
  EXPECT_EQ(actual, expected) << trial;
 }
}
}
