#include <gtest/gtest.h>
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
TEST(UnifiedLightRegistry, BlockEmissionClampsAndRoundTrips) {
 world::light::UnifiedLightRegistry::setBlockEmission(250, 15);
 EXPECT_EQ(world::light::UnifiedLightRegistry::blockEmission(250), 15);
 world::light::UnifiedLightRegistry::setBlockEmission(250, 99);
 EXPECT_EQ(world::light::UnifiedLightRegistry::blockEmission(250), 15);
 world::light::UnifiedLightRegistry::setBlockEmission(250, -4);
 EXPECT_EQ(world::light::UnifiedLightRegistry::blockEmission(250), 0);
 EXPECT_EQ(world::light::UnifiedLightRegistry::blockEmission(-1), 0);
 EXPECT_EQ(world::light::UnifiedLightRegistry::blockEmission(100000), 0);
}
TEST(UnifiedLightRegistry, BlockLightColorPacksAndUnpacks) {
 world::light::UnifiedLightRegistry::setBlockLightColor(120, 1.0f, 0.5f, 0.0f);
 float r = 0.0f;
 float g = 0.0f;
 float b = 0.0f;
 world::light::UnifiedLightRegistry::blockLightColor(120, r, g, b);
 EXPECT_FLOAT_EQ(r, 1.0f);
 EXPECT_NEAR(g, 0.5f, 1.0f / 255.0f);
 EXPECT_FLOAT_EQ(b, 0.0f);
 float dr = 0.0f;
 float dg = 0.0f;
 float db = 0.0f;
 world::light::UnifiedLightRegistry::blockLightColor(-1, dr, dg, db);
 EXPECT_FLOAT_EQ(dr, 1.0f);
 EXPECT_FLOAT_EQ(dg, 1.0f);
 EXPECT_FLOAT_EQ(db, 1.0f);
}
TEST(UnifiedLightRegistry, SunIsPerInstance) {
 world::light::UnifiedLightRegistry registry;
 world::light::SunLight sun;
 sun.directionX = 0.25f;
 sun.directionY = 0.75f;
 sun.intensity = 2.0f;
 registry.setSun(sun);
 EXPECT_FLOAT_EQ(registry.sun().directionX, 0.25f);
 EXPECT_FLOAT_EQ(registry.sun().directionY, 0.75f);
 EXPECT_FLOAT_EQ(registry.sun().intensity, 2.0f);
}
} // namespace net::minecraft::test
