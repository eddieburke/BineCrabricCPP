
#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
namespace net::minecraft::test {
namespace {
namespace ComputeDispatcher = net::minecraft::client::render::ComputeDispatcher;
using net::minecraft::client::render::indexPackPasses;
using net::minecraft::client::render::isProgramEnabled;
using net::minecraft::client::render::PackDefinition;
using net::minecraft::client::render::PackPass;
using net::minecraft::client::render::PackPassBuckets;
void chunkOffset(int sectionX, int sectionY, int sectionZ, double camX, double camY, double camZ,
                 float& ox, float& oy, float& oz) {
 ox = static_cast<float>(static_cast<double>(sectionX) - camX);
 oy = static_cast<float>(static_cast<double>(sectionY) - camY);
 oz = static_cast<float>(static_cast<double>(sectionZ) - camZ);
}
} // namespace
TEST(IrisChunkOffsetHemisphere, PositiveAndNegativeCameras) {
 float ox = 0.0f, oy = 0.0f, oz = 0.0f;
 chunkOffset(1600, 64, 1600, 1600.5, 70.0, 1600.5, ox, oy, oz);
 EXPECT_NEAR(ox, -0.5f, 1e-4f);
 EXPECT_NEAR(oy, -6.0f, 1e-4f);
 EXPECT_NEAR(oz, -0.5f, 1e-4f);
 chunkOffset(-1600, 64, -1600, 1600.5, 70.0, 1600.5, ox, oy, oz);
 EXPECT_LT(ox, -3000.0f);
 EXPECT_LT(oz, -3000.0f);
 chunkOffset(-1600, 64, -1600, -1600.5, 70.0, -1600.5, ox, oy, oz);
 EXPECT_NEAR(ox, 0.5f, 1e-4f);
 EXPECT_NEAR(oz, 0.5f, 1e-4f);
 chunkOffset(1600, 64, 1600, -1600.5, 70.0, -1600.5, ox, oy, oz);
 EXPECT_GT(ox, 3000.0f);
 EXPECT_GT(oz, 3000.0f);
}
TEST(IrisChunkOffsetHemisphere, NoRegionWrapArtifact) {
 float ox = 0.0f, oy = 0.0f, oz = 0.0f;
 chunkOffset(-16, 0, -16, -0.5, 0.0, -0.5, ox, oy, oz);
 EXPECT_NEAR(ox, -15.5f, 1e-4f);
 EXPECT_NEAR(oz, -15.5f, 1e-4f);
 EXPECT_NE(ox, 1008.5f);
 EXPECT_NE(oz, 1008.5f);
}
// The workGroups dispatch rules moved to tests/compute_dispatch_parity_test.cpp,
// which checks each of Java ComputeProgram.getWorkGroups's three branches against a
// transcription of the Java expression rather than two hand-picked cases, and takes
// the local size from the linked program the way Java does.
TEST(IrisComputeOrder, UnsuffixedThenLetters) {
 const auto order = [](const char* name) { return ComputeDispatcher::computePassOrder(name); };
 EXPECT_LT(order("composite"), order("composite_a"));
 EXPECT_LT(order("composite_a"), order("composite_z"));
 EXPECT_LT(order("composite_z"), order("composite1"));
 EXPECT_LT(order("composite1_z"), order("composite2"));
 EXPECT_NE(order("composite_a"), order("composite3_a"));
 EXPECT_TRUE(ComputeDispatcher::matchesStage("composite_a", "composite"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("deferred_a", "composite"));
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
 PackDefinition definition;
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
 settings["BLOOM"] = "off";
 EXPECT_TRUE(isProgramEnabled(definition, settings, "prepare"));
 settings["BLOOM"] = "FALSE";
 EXPECT_TRUE(isProgramEnabled(definition, settings, "prepare"));
 settings["BLOOM"] = "1";
 EXPECT_TRUE(isProgramEnabled(definition, settings, "prepare"));
 settings["BLOOM"] = "0";
 EXPECT_FALSE(isProgramEnabled(definition, settings, "prepare"));
 EXPECT_TRUE(isProgramEnabled(definition, settings, "gbuffers_terrain"));
}
TEST(IrisProgramEnabled, IndexesSkipDisabledPasses) {
 PackDefinition definition;
 PackPass post;
 post.name = "composite";
 post.type = "post";
 post.program = "composite";
 PackPass deferred;
 deferred.name = "deferred";
 deferred.type = "deferred";
 deferred.program = "deferred";
 definition.passes.push_back(post);
 definition.passes.push_back(deferred);
 definition.programEnabled["composite"] = "false";
 PackPassBuckets buckets;
 net::minecraft::client::render::ProgramEnabledCache cache;
 indexPackPasses(definition, {}, buckets, cache);
 EXPECT_TRUE(buckets.postPasses.empty());
 ASSERT_EQ(buckets.deferredPasses.size(), 1u);
 EXPECT_EQ(buckets.deferredPasses[0], 1u);
}
} // namespace net::minecraft::test
