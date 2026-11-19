#include <gtest/gtest.h>
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
namespace runtime = net::minecraft::mod::runtime;
TEST(ModRenderScope, CloudStageOwnsCloudShaderProgram) {
 EXPECT_EQ(runtime::parseModDrawLayer("clouds"), runtime::ModDrawLayer::Clouds);
 EXPECT_EQ(runtime::modDrawProgramKey(true, true, true, false, runtime::ModDrawLayer::Clouds),
           "gbuffers_clouds");
 EXPECT_EQ(runtime::modDrawProgramKey(true, true, true, false, runtime::ModDrawLayer::Auto),
           "gbuffers_particles_translucent");
}
TEST(ModRenderScope, ExplicitBasicLayerSurvivesDepthDisabledDraws) {
 EXPECT_EQ(runtime::modDrawProgramKey(false, true, false, false, runtime::ModDrawLayer::Basic),
           "gbuffers_basic");
 EXPECT_EQ(runtime::modDrawProgramKey(false, true, false, false, runtime::ModDrawLayer::Sky),
           "gbuffers_skybasic");
}
TEST(ModRenderScope, NestedStageLayerRestoresOuterLayer) {
 unsigned char worldStorage = 0;
 auto* world = reinterpret_cast<net::minecraft::World*>(&worldStorage);
 runtime::ScopedModWorldDrawContext outer(world, 0.25f, runtime::ModDrawLayer::Clouds);
 EXPECT_EQ(runtime::ModWorldDrawContext::layer(), runtime::ModDrawLayer::Clouds);
 {
  runtime::ScopedModWorldDrawContext inner(world, 0.5f, runtime::ModDrawLayer::Sky);
  EXPECT_EQ(runtime::ModWorldDrawContext::layer(), runtime::ModDrawLayer::Sky);
 }
 EXPECT_EQ(runtime::ModWorldDrawContext::layer(), runtime::ModDrawLayer::Clouds);
}
TEST(ModRenderScope, ParsesAllStandardBlendModesAndFactors) {
 const auto off = runtime::parseModBlendMode("none", true);
 EXPECT_FALSE(off.enabled);
 const auto alpha = runtime::parseModBlendMode("alpha", false);
 EXPECT_TRUE(alpha.enabled);
 EXPECT_EQ(alpha.src, 0x0302);
 EXPECT_EQ(alpha.dst, 0x0303);
 const auto add = runtime::parseModBlendMode("additive");
 EXPECT_TRUE(add.enabled);
 EXPECT_EQ(add.src, 0x0302);
 EXPECT_EQ(add.dst, 0x0001);
 const auto plus = runtime::parseModBlendMode("additive_one");
 EXPECT_EQ(plus.src, 0x0001);
 EXPECT_EQ(plus.dst, 0x0001);
 const auto screen = runtime::parseModBlendMode("screen");
 EXPECT_EQ(screen.src, 0x0307);
 EXPECT_EQ(screen.dst, 0x0001);
 const auto mult = runtime::parseModBlendMode("multiply");
 EXPECT_EQ(mult.src, 0x0306);
 EXPECT_EQ(mult.dst, 0x0000);
 const auto premult = runtime::parseModBlendMode("premultiplied");
 EXPECT_EQ(premult.src, 0x0001);
 EXPECT_EQ(premult.dst, 0x0303);
 EXPECT_EQ(runtime::parseModBlendFactor("one_minus_dst_alpha", 0), 0x0305);
}
