#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

namespace net::minecraft::test {
namespace {
struct WaterEntityLighting : testing::Test {
 ClientWorld world{nullptr, 12345, 0};
 entity::passive::CowEntity cow{&world};
 Chunk* chunk = nullptr;
 void SetUp() override {
  world.updateChunk(0, 0, true);
  chunk = &world.getChunk(0, 0);
  for(int y = 62; y <= 64; ++y) {
   chunk->blocks[(8 << 11) | (8 << 7) | y] = static_cast<std::uint8_t>(Block::WATER->id);
   chunk->skyLight.set(8, y, 8, (y - 60) * 3);
  }
  chunk->skyLight.set(8, 64, 8, 12);
  chunk->skyLight.set(8, 65, 8, 15);
  cow.setPosition(8.5, 63.95, 8.5);
 }
};
}

TEST_F(WaterEntityLighting, ExposedHeadDoesNotUseOnlySubmergedBodyLight) {
 ASSERT_FALSE(cow.isInFluid(block::material::Material::WATER));
 chunk->blockLight.set(8, 64, 8, 6);
 int block = 0;
 int sky = 0;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 6);
 EXPECT_EQ(sky, 15);
}

TEST_F(WaterEntityLighting, SubmergedHeadKeepsWaterAttenuation) {
 cow.setPosition(8.5, 62.9, 8.5);
 ASSERT_TRUE(cow.isInFluid(block::material::Material::WATER));
 int block = 0;
 int sky = 0;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 0);
 EXPECT_EQ(sky, 9);
}

TEST_F(WaterEntityLighting, ExposedHeadUsesActualLightUnderCover) {
 chunk->skyLight.set(8, 64, 8, 5);
 chunk->skyLight.set(8, 65, 8, 7);
 int block = 0;
 int sky = 0;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 0);
 EXPECT_EQ(sky, 7);
}

TEST_F(WaterEntityLighting, DryBodyKeepsExistingLightSample) {
 chunk->blocks[(8 << 11) | (8 << 7) | 64] = 0;
 int block = 0;
 int sky = 0;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 0);
 EXPECT_EQ(sky, 12);
}

TEST_F(WaterEntityLighting, ExposedHeadAboveShallowFlowInSameBlockUsesAirLight) {
 cow.setPosition(8.5, 63.2, 8.5);
 chunk->meta.set(8, 64, 8, 7);
 ASSERT_FALSE(cow.isInFluid(block::material::Material::WATER));
 int block = 0;
 int sky = 0;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 0);
 EXPECT_EQ(sky, 15);
}

TEST_F(WaterEntityLighting, MissingChunkIsNotLoadedByLightSampling) {
 cow.setPosition(48.5, 64.0, 48.5);
 ASSERT_FALSE(world.hasChunk(3, 3));
 int block = 15;
 int sky = 15;
 cow.sampleLightLevels(block, sky);
 EXPECT_EQ(block, 0);
 EXPECT_EQ(sky, 0);
 EXPECT_FALSE(world.hasChunk(3, 3));
}
}
