#include <gtest/gtest.h>
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
TEST(UnifiedLightRegistry, OwnsPointAndDirectionalSources) {
  world::light::UnifiedLightRegistry registry;
  world::light::UnifiedLightRegistry::setBlockEmission(250, 15);
  registry.syncBlockSource(250, 4, 5, 6);
  world::light::PhysicalLight sun;
  sun.shape = world::light::LightShape::Directional;
  sun.directionX = 0.25f;
  sun.directionY = 0.75f;
  sun.intensity = 2.0f;
  registry.upsert(world::light::UnifiedLightRegistry::sunKey(), sun);
  auto view = registry.read();
  ASSERT_EQ(view.sources().size(), 2U);
  const auto* point = view.find(world::light::UnifiedLightRegistry::blockKey(4, 5, 6));
  ASSERT_NE(point, nullptr);
  EXPECT_EQ(point->shape, world::light::LightShape::Point);
  EXPECT_DOUBLE_EQ(point->x, 4.5);
  const auto* directional = view.find(world::light::UnifiedLightRegistry::sunKey());
  ASSERT_NE(directional, nullptr);
  EXPECT_EQ(directional->shape, world::light::LightShape::Directional);
  EXPECT_FLOAT_EQ(directional->intensity, 2.0f);
  world::light::UnifiedLightRegistry::setBlockEmission(250, 0);
}
TEST(UnifiedLightRegistry, HasNoEngineSourceCap) {
  world::light::UnifiedLightRegistry registry;
  constexpr std::uint64_t count = 10000;
  for(std::uint64_t id = 0; id < count; ++id) {
    world::light::PhysicalLight source;
    source.x = static_cast<double>(id);
    registry.upsert({world::light::LightDomain::Native, 0, 0, 0, id}, source);
  }
  EXPECT_EQ(registry.read().sources().size(), count);
  EXPECT_TRUE(registry.erase({world::light::LightDomain::Native, 0, 0, 0, count / 2}));
  EXPECT_EQ(registry.read().sources().size(), count - 1);
}
}
