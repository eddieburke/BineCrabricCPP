#include <gtest/gtest.h>
#include <cstddef>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
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
} // namespace net::minecraft::test
