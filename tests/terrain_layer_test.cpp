#include <gtest/gtest.h>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
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
} // namespace net::minecraft::test
