// Regression test: landing in water must not deal fall damage.
// Java: Entity.fall() resets fallDistance while the entity is in water, so
// onLanding (and LivingEntity::onLanding's damage) never fires.
#include <cmath>
#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

namespace net::minecraft::test {
namespace {
class RecordingLivingEntity : public entity::LivingEntity {
 public:
 explicit RecordingLivingEntity(World* world) : LivingEntity(world) {
 }
 void onLanding(float fallDistanceIn) override {
  ++landingCount;
  lastLandingDistance = fallDistanceIn;
  LivingEntity::onLanding(fallDistanceIn);
 }
 int landingCount = 0;
 float lastLandingDistance = -1.0f;
};

struct WaterFallDamage : testing::Test {
 ClientWorld world{nullptr, 12345, 0};
 Chunk* chunk = nullptr;
 void SetUp() override {
  world.updateChunk(0, 0, true);
  chunk = &world.getChunk(0, 0);
  for(int y = 62; y <= 64; ++y) {
   chunk->blocks[(8 << 11) | (8 << 7) | y] = static_cast<std::uint8_t>(Block::WATER->id);
  }
 }
};
} // namespace

TEST_F(WaterFallDamage, LandingInWaterDealsNoDamage) {
 entity::RecordingLivingEntity entity{&world};
 entity.setBoundingBoxSpacing(0.6f, 1.8f);
 // Feet at 63.0, eyes (~y + eyeHeight) inside the water column at block y 63/64.
 entity.setPosition(8.5, 64.8, 8.5);
 ASSERT_TRUE(entity.isInFluid(block::material::Material::WATER));
 const int healthBefore = entity.health;
 entity.fallDistance = 20.0f;
 entity.fall(-5.0, true);
 EXPECT_EQ(entity.landingCount, 0);
 EXPECT_FLOAT_EQ(entity.fallDistance, 0.0f);
 EXPECT_EQ(entity.health, healthBefore);
}

TEST_F(WaterFallDamage, AccumulatesNoFallDistanceWhileFallingThroughWater) {
 entity::RecordingLivingEntity entity{&world};
 entity.setBoundingBoxSpacing(0.6f, 1.8f);
 entity.setPosition(8.5, 64.8, 8.5);
 ASSERT_TRUE(entity.isInFluid(block::material::Material::WATER));
 entity.fall(-1.0, false);
 EXPECT_FLOAT_EQ(entity.fallDistance, 0.0f);
}

TEST_F(WaterFallDamage, LandingOnDryGroundStillDamages) {
 entity::RecordingLivingEntity entity{&world};
 entity.setBoundingBoxSpacing(0.6f, 1.8f);
 entity.setPosition(8.5, 64.8, 8.5);
 // Remove the water so the entity lands on dry ground.
 for(int y = 62; y <= 64; ++y) {
  chunk->blocks[(8 << 11) | (8 << 7) | y] = 0;
 }
 ASSERT_FALSE(entity.isInFluid(block::material::Material::WATER));
 const int healthBefore = entity.health;
 entity.fallDistance = 20.0f;
 entity.fall(-5.0, true);
 EXPECT_EQ(entity.landingCount, 1);
 EXPECT_FLOAT_EQ(entity.lastLandingDistance, 20.0f);
 EXPECT_FLOAT_EQ(entity.fallDistance, 0.0f);
 EXPECT_LT(entity.health, healthBefore);
}
} // namespace net::minecraft::test
