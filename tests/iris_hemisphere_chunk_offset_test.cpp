// Wave 2 regression: Iris chunkOffset must be sectionOrigin - camera with no
// 1024-region wrap, so both world hemispheres draw correctly.
#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/shaderpack/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPassScheduler.hpp"
namespace net::minecraft::test {
namespace {
namespace ComputeDispatcher = net::minecraft::client::render::shaderpack::ComputeDispatcher;
using net::minecraft::client::render::shaderpack::indexShaderPasses;
using net::minecraft::client::render::shaderpack::isProgramEnabled;
using net::minecraft::client::render::shaderpack::ShaderPackDefinition;
using net::minecraft::client::render::shaderpack::ShaderPass;
using net::minecraft::client::render::shaderpack::ShaderPassBuckets;
void chunkOffset(int sectionX, int sectionY, int sectionZ, double camX, double camY, double camZ,
                 float& ox, float& oy, float& oz) {
 // Mirrors WorldRenderer::renderChunksVbo Iris path.
 ox = static_cast<float>(static_cast<double>(sectionX) - camX);
 oy = static_cast<float>(static_cast<double>(sectionY) - camY);
 oz = static_cast<float>(static_cast<double>(sectionZ) - camZ);
}
} // namespace
TEST(IrisChunkOffsetHemisphere, PositiveAndNegativeCameras) {
 float ox = 0.0f, oy = 0.0f, oz = 0.0f;
 // Camera deep in +X/+Z: nearby section offset stays near zero, far negative
 // section is large negative — never wrapped into 0..1023 region space.
 chunkOffset(1600, 64, 1600, 1600.5, 70.0, 1600.5, ox, oy, oz);
 EXPECT_NEAR(ox, -0.5f, 1e-4f);
 EXPECT_NEAR(oy, -6.0f, 1e-4f);
 EXPECT_NEAR(oz, -0.5f, 1e-4f);
 chunkOffset(-1600, 64, -1600, 1600.5, 70.0, 1600.5, ox, oy, oz);
 EXPECT_LT(ox, -3000.0f);
 EXPECT_LT(oz, -3000.0f);
 // Camera deep in −X/−Z.
 chunkOffset(-1600, 64, -1600, -1600.5, 70.0, -1600.5, ox, oy, oz);
 EXPECT_NEAR(ox, 0.5f, 1e-4f);
 EXPECT_NEAR(oz, 0.5f, 1e-4f);
 chunkOffset(1600, 64, 1600, -1600.5, 70.0, -1600.5, ox, oy, oz);
 EXPECT_GT(ox, 3000.0f);
 EXPECT_GT(oz, 3000.0f);
}
TEST(IrisChunkOffsetHemisphere, NoRegionWrapArtifact) {
 float ox = 0.0f, oy = 0.0f, oz = 0.0f;
 // Legacy display-list path used x & 0x3FF; that maps -16 → 1008 and breaks
 // hemisphere draws. Iris offset must stay continuous around the origin.
 chunkOffset(-16, 0, -16, -0.5, 0.0, -0.5, ox, oy, oz);
 EXPECT_NEAR(ox, -15.5f, 1e-4f);
 EXPECT_NEAR(oz, -15.5f, 1e-4f);
 // Not the wrapped legacy local coordinate.
 EXPECT_NE(ox, 1008.5f);
 EXPECT_NE(oz, 1008.5f);
}
TEST(IrisComputeWorkGroups, DefaultsMatchSpec) {
 ShaderPass pass;
 // Defaults: relativeGroups=true, groupScale=1, localSize=1 → ceil(view/1).
 const auto groups = ComputeDispatcher::workGroups(pass, 1920, 1080);
 EXPECT_EQ(groups[0], 1920u);
 EXPECT_EQ(groups[1], 1080u);
 EXPECT_EQ(groups[2], 1u);
}
TEST(IrisComputeWorkGroups, RelativeScaleAndLocalSize) {
 ShaderPass pass;
 pass.groupScale[0] = 0.5f;
 pass.groupScale[1] = 0.5f;
 pass.localSize[0] = 8;
 pass.localSize[1] = 8;
 const auto groups = ComputeDispatcher::workGroups(pass, 100, 50);
 EXPECT_EQ(groups[0], static_cast<unsigned>(std::ceil(100.0f * 0.5f / 8.0f)));
 EXPECT_EQ(groups[1], static_cast<unsigned>(std::ceil(50.0f * 0.5f / 8.0f)));
 EXPECT_EQ(groups[2], 1u);
}
TEST(IrisComputeOrder, UnsuffixedThenLetters) {
 EXPECT_EQ(ComputeDispatcher::computePassOrder("composite"), -1);
 EXPECT_EQ(ComputeDispatcher::computePassOrder("composite_a"), 0);
 EXPECT_EQ(ComputeDispatcher::computePassOrder("composite_z"), 25);
 EXPECT_TRUE(ComputeDispatcher::attachedToPass("composite_a", "composite"));
 EXPECT_FALSE(ComputeDispatcher::attachedToPass("deferred_a", "composite"));
}
TEST(IrisComputeOrder, ParentNameAndNumericOrder) {
 EXPECT_EQ(ComputeDispatcher::computeParentName("composite"), "composite");
 EXPECT_EQ(ComputeDispatcher::computeParentName("composite3_a"), "composite3");
 EXPECT_EQ(ComputeDispatcher::computeParentName("composite3_b"), "composite3");
 EXPECT_TRUE(ComputeDispatcher::lessComputeParent("composite", "composite1"));
 EXPECT_TRUE(ComputeDispatcher::lessComputeParent("composite2", "composite10"));
 EXPECT_FALSE(ComputeDispatcher::lessComputeParent("composite10", "composite2"));
}
TEST(IrisProgramEnabled, TrueFalseAndOption) {
 ShaderPackDefinition definition;
 definition.programEnabled["composite"] = "false";
 definition.programEnabled["deferred"] = "true";
 definition.programEnabled["prepare"] = "BLOOM";
 std::unordered_map<std::string, std::string> settings{{"BLOOM", "true"}};
 EXPECT_FALSE(isProgramEnabled(definition, settings, "composite"));
 EXPECT_FALSE(isProgramEnabled(definition, settings, "composite#compute"));
 EXPECT_TRUE(isProgramEnabled(definition, settings, "deferred"));
 EXPECT_TRUE(isProgramEnabled(definition, settings, "prepare"));
 settings["BLOOM"] = "false";
 EXPECT_FALSE(isProgramEnabled(definition, settings, "prepare"));
 EXPECT_TRUE(isProgramEnabled(definition, settings, "gbuffers_terrain"));
}
TEST(IrisProgramEnabled, IndexesSkipDisabledPasses) {
 ShaderPackDefinition definition;
 ShaderPass post;
 post.name = "composite";
 post.type = "post";
 post.program = "composite";
 ShaderPass deferred;
 deferred.name = "deferred";
 deferred.type = "deferred";
 deferred.program = "deferred";
 definition.passes.push_back(post);
 definition.passes.push_back(deferred);
 definition.programEnabled["composite"] = "false";
 ShaderPassBuckets buckets;
 indexShaderPasses(definition, {}, buckets);
 EXPECT_TRUE(buckets.postPasses.empty());
 ASSERT_EQ(buckets.deferredPasses.size(), 1u);
 EXPECT_EQ(buckets.deferredPasses[0], 1u);
}
} // namespace net::minecraft::test
