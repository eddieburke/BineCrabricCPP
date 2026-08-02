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
