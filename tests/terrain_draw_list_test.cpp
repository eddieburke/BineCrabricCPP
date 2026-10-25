#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
#define private public
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#undef private

namespace net::minecraft::test {
namespace {
namespace render = client::render;
struct DrawListWorld {
 std::vector<block::entity::BlockEntity*> blockEntities;
 std::vector<std::unique_ptr<render::chunk::TerrainRegion>> regions;
 std::vector<std::unique_ptr<render::chunk::ChunkBuilder>> sections;
 std::vector<render::chunk::ChunkBuilder*> visible;
 explicit DrawListWorld(int regionCount, int sectionsPerRegion) {
  for(int regionIndex = 0; regionIndex < regionCount; ++regionIndex) {
   auto region = std::make_unique<render::chunk::TerrainRegion>(regionIndex * 128, 0, 0);
   for(int index = 0; index < sectionsPerRegion; ++index) {
    auto section = std::make_unique<render::chunk::ChunkBuilder>(
        nullptr, blockEntities, regionIndex * 128, index * 16, 0);
    section->setTerrainRegion(*region);
    for(auto& allocation : section->terrainAllocations_) {
     allocation.capacityVertices = 256;
     allocation.vertexCount = 64;
    }
    visible.push_back(section.get());
    sections.push_back(std::move(section));
   }
   regions.push_back(std::move(region));
  }
  std::uint32_t random = 99991;
  for(std::size_t i = visible.size(); i > 1; --i) {
   random = random * 1664525U + 1013904223U;
   std::swap(visible[i - 1], visible[random % static_cast<std::uint32_t>(i)]);
  }
 }
};
void checkDrawLists(render::WorldRenderer& renderer,
                    const std::vector<render::chunk::ChunkBuilder*>& visible) {
 for(int layer = 0; layer < render::chunk::terrain_layer::Count; ++layer) {
  const auto& list = renderer.terrainDrawLists_[static_cast<std::size_t>(layer)];
  std::size_t total = 0;
  std::vector<render::chunk::TerrainRegion*> seen;
  for(std::size_t i = 0; i < list.regionCount; ++i) {
   const auto& draw = list.regions[i];
   EXPECT_EQ(std::find(seen.begin(), seen.end(), draw.region), seen.end());
   seen.push_back(draw.region);
   std::vector<const render::chunk::TerrainAllocation*> expected;
   for(auto* section : visible) {
    if(section != nullptr && section->terrainRegion() == draw.region && section->terrainAllocation(layer).valid()) {
     expected.push_back(&section->terrainAllocation(layer));
    }
   }
   if(layer == render::chunk::terrain_layer::Translucent) std::reverse(expected.begin(), expected.end());
   EXPECT_EQ(draw.allocations, expected);
   total += draw.allocations.size();
  }
  std::size_t expectedTotal = 0;
  for(auto* section : visible) {
   if(section != nullptr && section->terrainRegion() != nullptr && section->terrainAllocation(layer).valid()) {
    ++expectedTotal;
   }
  }
  EXPECT_EQ(total, expectedTotal);
 }
}
}
TEST(TerrainDrawList, CachedRegionIndicesSurviveReorderingAndScopedPasses) {
 DrawListWorld world(12, 8);
 render::WorldRenderer renderer;
 render::WorldRenderer other;
 for(int round = 0; round < 8; ++round) {
  for(auto& region : world.regions) region->drawListIndices.fill(std::numeric_limits<std::size_t>::max());
  renderer.chunkSections_.regularVisibleSections_ = world.visible;
  renderer.buildTerrainDrawLists({0, 0, 0});
  checkDrawLists(renderer, world.visible);
  std::reverse(world.visible.begin(), world.visible.end());
  other.chunkSections_.regularVisibleSections_ = world.visible;
  other.buildTerrainDrawLists({1024, 0, 0});
  checkDrawLists(other, world.visible);
  renderer.chunkSections_.useScopedVisibleSections(true);
  auto scoped = std::vector<render::chunk::ChunkBuilder*>(world.visible.begin(), world.visible.begin() + 23);
  renderer.chunkSections_.scopedVisibleSections_ = scoped;
  renderer.buildTerrainDrawLists({512, 0, 0});
  checkDrawLists(renderer, scoped);
  renderer.chunkSections_.useScopedVisibleSections(false);
  renderer.buildTerrainDrawLists({0, 0, 0});
  checkDrawLists(renderer, renderer.chunkSections_.regularVisibleSections_);
  std::rotate(world.visible.begin(), world.visible.begin() + 7, world.visible.end());
 }
}
TEST(TerrainDrawList, EmptyLayersAndDisappearingRegionsLeaveNoStaleDraws) {
 DrawListWorld world(6, 4);
 render::WorldRenderer renderer;
 renderer.chunkSections_.regularVisibleSections_ = world.visible;
 renderer.buildTerrainDrawLists({0, 0, 0});
 for(auto& section : world.sections) section->terrainAllocations_[1] = {};
 renderer.chunkSections_.regularVisibleSections_.resize(9);
 renderer.chunkSections_.regularVisibleSections_.push_back(nullptr);
 renderer.buildTerrainDrawLists({0, 0, 0});
 checkDrawLists(renderer, renderer.chunkSections_.regularVisibleSections_);
 EXPECT_EQ(renderer.terrainDrawLists_[1].regionCount, 0u);
 renderer.chunkSections_.regularVisibleSections_.clear();
 renderer.buildTerrainDrawLists({0, 0, 0});
 for(const auto& list : renderer.terrainDrawLists_) EXPECT_EQ(list.regionCount, 0u);
}
TEST(TerrainDrawList, ExtremeDistanceGroupingThroughput) {
 DrawListWorld world(128, 256);
 render::WorldRenderer renderer;
 renderer.chunkSections_.regularVisibleSections_ = world.visible;
 std::array<render::WorldRenderer::TerrainDrawList, render::chunk::terrain_layer::Count> legacy;
 const auto linearBuild = [&] {
  for(auto& list : legacy) list.regionCount = 0;
  std::array<render::chunk::TerrainRegion*, render::chunk::terrain_layer::Count> lastRegions{};
  std::array<std::size_t, render::chunk::terrain_layer::Count> lastIndices{};
  for(auto* section : world.visible) {
   auto* region = section->terrainRegion();
   for(int layer = 0; layer < render::chunk::terrain_layer::Count; ++layer) {
    const auto index = static_cast<std::size_t>(layer);
    auto& list = legacy[index];
    auto regionIndex = lastIndices[index];
    if(region != lastRegions[index]) {
     regionIndex = 0;
     while(regionIndex < list.regionCount && list.regions[regionIndex].region != region) ++regionIndex;
     if(regionIndex == list.regionCount) {
      ++list.regionCount;
      if(regionIndex == list.regions.size()) list.regions.push_back({});
      list.regions[regionIndex].region = region;
      list.regions[regionIndex].allocations.clear();
     }
     lastRegions[index] = region;
     lastIndices[index] = regionIndex;
    }
    list.regions[regionIndex].allocations.push_back(&section->terrainAllocation(layer));
   }
  }
 };
 linearBuild();
 renderer.buildTerrainDrawLists({0, 0, 0});
 double linearMs = 0;
 double indexedMs = 0;
 constexpr int rounds = 60;
 for(int round = 0; round < rounds; ++round) {
  const auto run = [](auto&& operation) {
   const auto start = std::chrono::steady_clock::now();
   operation();
   return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  };
  if(round % 2 == 0) {
   linearMs += run(linearBuild);
   indexedMs += run([&] { renderer.buildTerrainDrawLists({0, 0, 0}); });
  } else {
   indexedMs += run([&] { renderer.buildTerrainDrawLists({0, 0, 0}); });
   linearMs += run(linearBuild);
  }
 }
 std::printf("[region grouping] %zu sections, 128 regions, 4 layers: linear %.3f ms, indexed %.3f ms\n",
             world.visible.size(), linearMs / rounds, indexedMs / rounds);
 checkDrawLists(renderer, world.visible);
}
}
