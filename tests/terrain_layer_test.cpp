#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::test {
TEST(TerrainLayerTest, GrassAndLeavesUseAlphaTestedTerrainPrograms) {
 using namespace client::render::chunk;
 ASSERT_NE(block::Block::GRASS_BLOCK, nullptr);
 ASSERT_NE(block::Block::LEAVES, nullptr);
 EXPECT_EQ(resolveTerrainMeshLayer(*block::Block::GRASS_BLOCK, block::Block::GRASS_BLOCK->id, {}),
           terrain_layer::Cutout);
 EXPECT_EQ(resolveTerrainMeshLayer(*block::Block::LEAVES, block::Block::LEAVES->id, {}),
           terrain_layer::Cutout);
}
TEST(TerrainLayerTest, ShaderPackLayerOverrideRemainsAuthoritative) {
 using namespace client::render::chunk;
 ASSERT_NE(block::Block::GRASS_BLOCK, nullptr);
 const std::unordered_map<int, int> overrides{{block::Block::GRASS_BLOCK->id, terrain_layer::Solid}};
 EXPECT_EQ(resolveTerrainMeshLayer(*block::Block::GRASS_BLOCK, block::Block::GRASS_BLOCK->id, overrides),
           terrain_layer::Solid);
}
TEST(TerrainLayerTest, LeafOpacityTableTracksTheGraphicsLevel) {
 ASSERT_NE(block::Block::LEAVES, nullptr);
 auto& leaves = *static_cast<block::LeavesBlock*>(block::Block::LEAVES);
 const bool restore = leaves.renderSides;
 const auto id = static_cast<std::size_t>(leaves.id);
 leaves.setFancyGraphics(true);
 EXPECT_FALSE(block::Block::BLOCKS_OPAQUE[id]);
 leaves.setFancyGraphics(false);
 EXPECT_TRUE(block::Block::BLOCKS_OPAQUE[id]);
 leaves.setFancyGraphics(restore);
 EXPECT_EQ(block::Block::BLOCKS_LIGHT_OPACITY[id], 1);
}
TEST(TerrainLayerTest, FancyLeavesRenderLeafToLeafFacesLikeBetaBlockLeavesBase) {
 ASSERT_NE(block::Block::LEAVES, nullptr);
 ASSERT_NE(block::Block::GLASS, nullptr);
 auto& leaves = *static_cast<block::LeavesBlock*>(block::Block::LEAVES);
 const bool restore = leaves.renderSides;
 Chunk chunk(nullptr, 0, 0);
 const auto put = [&](int x, int y, int z, int id) {
  chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = static_cast<std::uint8_t>(id);
 };
 put(8, 64, 8, leaves.id);
 put(9, 64, 8, leaves.id);
 put(8, 64, 9, block::Block::GLASS->id);
 put(8, 64, 10, block::Block::GLASS->id);
 const std::array sources{client::render::chunk::RegionSnapshot::SourceChunk{0, 0, &chunk}};
 client::render::chunk::RegionSnapshot snapshot(sources, 0, {}, nullptr, 0, 63, 0, 15, 65, 15);
 const Box unit{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
 leaves.setFancyGraphics(true);
 EXPECT_TRUE(leaves.isSideVisibleForBounds(&snapshot, 9, 64, 8, 5, unit));
 EXPECT_TRUE(leaves.isSideVisibleForBounds(&snapshot, 8, 64, 8, 4, unit));
 leaves.setFancyGraphics(false);
 EXPECT_FALSE(leaves.isSideVisibleForBounds(&snapshot, 9, 64, 8, 5, unit));
 leaves.setFancyGraphics(restore);
 EXPECT_FALSE(block::Block::GLASS->isSideVisibleForBounds(&snapshot, 8, 64, 10, 3, unit));
}
} // namespace net::minecraft::test
