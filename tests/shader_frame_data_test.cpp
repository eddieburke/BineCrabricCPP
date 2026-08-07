#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/BiomeNames.hpp"
namespace net::minecraft::test {
namespace {
bool identity(const float* matrix) {
 static constexpr float expected[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f,
                                        0.0f, 0.0f, 0.0f, 1.0f};
 return std::equal(std::begin(expected), std::end(expected), matrix);
}
std::string read(const std::filesystem::path& path) {
 std::ifstream input(path, std::ios::binary);
 return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
} // namespace
TEST(ShaderUniformDefaults, MatricesAreInvertibleBeforeFirstWorldFrame) {
 const client::render::PackUniformValues values;
 EXPECT_TRUE(identity(values.gbufferProjection));
 EXPECT_TRUE(identity(values.gbufferProjectionInverse));
 EXPECT_TRUE(identity(values.gbufferPreviousProjection));
 EXPECT_TRUE(identity(values.gbufferModelView));
 EXPECT_TRUE(identity(values.gbufferModelViewInverse));
 EXPECT_TRUE(identity(values.gbufferPreviousModelView));
 EXPECT_TRUE(identity(values.shadowModelView));
 EXPECT_TRUE(identity(values.shadowModelViewInverse));
 EXPECT_TRUE(identity(values.shadowProjection));
 EXPECT_TRUE(identity(values.shadowProjectionInverse));
}
TEST(ShaderFrameData, AdvancesOnRenderedFramesInsteadOfWorldTicks) {
 client::render::FrameRenderCamera camera;
 client::render::FrameRenderCamera shadow;
 const auto first = client::render::buildShaderFrameData(
     1280, 720, 42.25f, 0, false, false, camera, shadow, nullptr);
 camera.projectionX = 1.5f;
 const auto second = client::render::buildShaderFrameData(
     1280, 720, 42.25f, 0, false, false, camera, shadow, nullptr);
 EXPECT_EQ(second.frameCounter, (first.frameCounter + 1) % 720720);
 EXPECT_GE(second.frameTime, 0.0f);
 EXPECT_GE(second.frameTimeCounter, first.frameTimeCounter);
 EXPECT_TRUE(std::equal(std::begin(first.gbufferProjection), std::end(first.gbufferProjection),
                        std::begin(second.gbufferPreviousProjection)));
}
TEST(ShaderFrameData, WrapsXzCameraPositionAndKeepsExactSplit) {
 client::render::FrameRenderCamera camera;
 client::render::FrameRenderCamera shadow;
 camera.eyeX = 30001.25;
 camera.eyeY = -30001.75;
 camera.eyeZ = 60002.5;
 const auto values = client::render::buildShaderFrameData(
     1280, 720, 0.0f, 0, false, false, camera, shadow, nullptr);
 // Java CameraPositionTracker shifts X/Z only (CameraUniforms.java:104-126); Y is
 // never shifted, so the shader camera Y stays the raw world coordinate.
 EXPECT_FLOAT_EQ(values.cameraPosition[0], 1.25f);
 EXPECT_FLOAT_EQ(values.cameraPosition[1], -30001.75f);
 EXPECT_FLOAT_EQ(values.cameraPosition[2], 2.5f);
 EXPECT_EQ(values.cameraPositionInt[0], 30001);
 EXPECT_EQ(values.cameraPositionInt[1], -30002);
 EXPECT_EQ(values.cameraPositionInt[2], 60002);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[0], 0.25f);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[1], 0.25f);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[2], 0.5f);
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
// getSunAngle(true)/360 = (SUN_ANGLE + 90) mod 360 with a single > 360 wrap, over 360.
TEST(ShaderFrameData, SunAngleMatchesIrisCelestialUniforms) {
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.0f), 0.25f);
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.25f), 0.5f);
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.5f), 0.75f);
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.9f), 0.15f);
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.75f), 1.0f);
 EXPECT_FLOAT_EQ(client::render::celestialSunAngle(0.99f), 0.24f);
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/transforms/SmoothedFloat.java
// s_t = lerp(s_{t-1}, x_t, 1 - e^(-ln(2) * frameTime / halfLife)) with the first value raw.
TEST(ShaderFrameData, SmoothExponentialMatchesSmoothedFloat) {
 float accumulator = 0.0f;
 bool initialized = false;
 EXPECT_FLOAT_EQ(client::render::smoothExponential(1.0f, accumulator, initialized, 1.0f, 1.0f),
                 1.0f);
 const float alpha = 1.0f - std::exp(-0.693147f);
 EXPECT_NEAR(client::render::smoothExponential(0.0f, accumulator, initialized, 1.0f, 1.0f),
             1.0f - alpha, 1e-5f);
 EXPECT_NEAR(client::render::smoothExponential(0.0f, accumulator, initialized, 1.0f, 1.0f),
             (1.0f - alpha) * (1.0f - alpha), 1e-5f);
}
TEST(ShaderFrameData, SmoothExponentialSeedsInstantlyForZeroHalflife) {
 float accumulator = 0.5f;
 bool initialized = true;
 EXPECT_FLOAT_EQ(client::render::smoothExponential(0.25f, accumulator, initialized, 1.0f, 0.0f),
                 0.25f);
}
// Java CommonUniforms: wetness = SmoothedFloat(wetnessHalflife, drynessHalflife, rainStrength)
// with half lives in deciseconds; the first frame seeds the raw rain strength.
TEST(ShaderFrameData, WetnessSmoothSeedsAndUsesDirectiveHalflife) {
 EXPECT_FLOAT_EQ(client::render::updateWetnessSmooth(1.0f, 0.0f, 600.0f, 200.0f), 1.0f);
 const float alpha = 1.0f - std::exp(-std::log(2.0f) / (200.0f * 0.1f));
 EXPECT_NEAR(client::render::updateWetnessSmooth(0.5f, 1.0f, 600.0f, 200.0f),
             1.0f + (0.5f - 1.0f) * alpha, 1e-4f);
}
// Java CenterDepthSampler: linearizes the window depth and smooths with decay
// 1 / ((centerDepthHalflife * 0.1) / LN2); the accumulator starts at zero.
TEST(ShaderFrameData, CenterDepthSmoothLinearizesAndSmooths) {
 const float result = client::render::updateCenterDepthSmooth(0.5f, 0.05f, 256.0f, 1.0f, 1.0f);
 const float linear = (0.05f * 256.0f) / (256.0f + 0.5f * (0.05f - 256.0f));
 const float alpha = 1.0f - std::exp(-std::log(2.0f) / (1.0f * 0.1f));
 EXPECT_NEAR(result, linear * alpha, 1e-4f);
}
TEST(ShaderFrameData, BiomeUniformNumberingMatchesBetaRegistryOrder) {
 // Java: MixinBiomes.iris$registerBiome assigns SEQUENTIAL ids (currentId++) in
 // registry order, BiomeUniforms.java:30-31 uploads biomeMap.getInt(...) and
 // IrisDefines.java:28 emits one "#define BIOME_<name> <id>" per registered biome.
 // There is NO pack biome.* parsing (uniforms.md's earlier claim was wrong).
 // C++: the biome uniform is static_cast<int>(biome.id) (FrameData.cpp:61) and the
 // BiomeId enum (Biome.hpp:10-24) IS the beta 1.7.3 registration order; kBiomes in
 // SourceProcessor.cpp:750-752 emits the BIOME_* defines for the same 13 slots.
 // Pin the enum ordinals so the uniform ids and define ids stay identical.
 EXPECT_EQ(static_cast<int>(BiomeId::Rainforest), 0);
 EXPECT_EQ(static_cast<int>(BiomeId::Swampland), 1);
 EXPECT_EQ(static_cast<int>(BiomeId::SeasonalForest), 2);
 EXPECT_EQ(static_cast<int>(BiomeId::Forest), 3);
 EXPECT_EQ(static_cast<int>(BiomeId::Savanna), 4);
 EXPECT_EQ(static_cast<int>(BiomeId::Shrubland), 5);
 EXPECT_EQ(static_cast<int>(BiomeId::Taiga), 6);
 EXPECT_EQ(static_cast<int>(BiomeId::Desert), 7);
 EXPECT_EQ(static_cast<int>(BiomeId::Plains), 8);
 EXPECT_EQ(static_cast<int>(BiomeId::IceDesert), 9);
 EXPECT_EQ(static_cast<int>(BiomeId::Tundra), 10);
 EXPECT_EQ(static_cast<int>(BiomeId::Hell), 11);
 EXPECT_EQ(static_cast<int>(BiomeId::Sky), 12);
 EXPECT_EQ(kBiomeCount, 13);
}
TEST(ShaderFrameData, BiomeWireNamesMatchBetaRegistrationOrder) {
 // The registry (Biome.cpp init(), beta 1.7.3 order) assigns each enum ordinal its
 // wire name; biomeWireName(i) is what the world/atlas path uses for id i.
 static constexpr std::array<std::string_view, 13> kExpected = {
     "rainforest", "swampland", "seasonal_forest", "forest", "savanna",
     "shrubland", "taiga", "desert", "plains", "ice_desert",
     "tundra", "hell", "sky"};
 for(int i = 0; i < kBiomeCount; ++i) {
  EXPECT_EQ(biomeWireName(i), kExpected[static_cast<std::size_t>(i)]) << i;
 }
}
TEST(ShaderFrameData, VersionPreambleEmitsBiomeDefinesMatchingUniformIds) {
 // End-to-end pin of the BIOME_* defines: versionPreamble writes
 // "#define BIOME_<name> <id>" for each kBiomes slot (SourceProcessor.cpp:750-755).
 // Slot 11/12 use the modern key paths (nether_wastes/the_end) while the beta wire
 // names (hell/sky) stay resolvable through the custom-uniform alias table
 // (CustomUniforms.cpp lookupBuiltin); the ids are the beta registration ordinals.
 static constexpr std::array<std::string_view, 13> kBiomeDefineNames = {
     "RAINFOREST", "SWAMP", "SEASONAL_FOREST", "FOREST", "SAVANNA",
     "SHRUBLAND", "TAIGA", "DESERT", "PLAINS", "ICE_DESERT",
     "TUNDRA", "NETHER_WASTES", "THE_END"};
 const client::render::PackDefinition pack;
 const std::string preamble = client::render::versionPreamble(pack, "void main(){}");
 for(std::size_t i = 0; i < kBiomeDefineNames.size(); ++i) {
  EXPECT_NE(preamble.find("#define BIOME_" + std::string(kBiomeDefineNames[i]) + " " +
                          std::to_string(i) + "\n"),
            std::string::npos)
      << kBiomeDefineNames[i];
 }
 // CAT_ defines pin the biome_category numbering (BiomeCategories.java ordinals);
 // FrameData.cpp biomeData assigns category ids for all 13 beta biomes — every one
 // must be present in the emitted list.
 static constexpr std::array<std::string_view, 17> kCategoryNames = {
     "NONE", "TAIGA", "EXTREME_HILLS", "JUNGLE", "MESA", "PLAINS", "SAVANNA", "ICY",
     "THE_END", "BEACH", "FOREST", "OCEAN", "DESERT", "RIVER", "SWAMP", "MUSHROOM", "NETHER"};
 for(std::size_t i = 0; i < kCategoryNames.size(); ++i) {
  EXPECT_NE(preamble.find("#define CAT_" + std::string(kCategoryNames[i]) + " " +
                          std::to_string(i) + "\n"),
            std::string::npos)
      << kCategoryNames[i];
 }
}
TEST(VanillaShaderAbi, GeometryUsesPerDrawCoreMatrices) {
 const std::filesystem::path shaders =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "vanilla" / "shaders";
 const std::vector<std::string> names = {
     "gbuffers_basic.vsh", "gbuffers_entities.vsh", "gbuffers_gui.vsh",
     "gbuffers_gui_textured.vsh", "gbuffers_hand.vsh", "gbuffers_item.vsh",
     "gbuffers_line.vsh", "gbuffers_skybasic.vsh", "gbuffers_skytextured.vsh",
     "gbuffers_terrain.vsh", "gbuffers_text.vsh", "gbuffers_textured.vsh",
     "gbuffers_textured_lit.vsh", "gbuffers_water.vsh"};
 for(const std::string& name : names) {
  const std::string source = read(shaders / name);
  EXPECT_FALSE(source.empty()) << name;
  EXPECT_NE(source.find("modelViewMatrix"), std::string::npos) << name;
  EXPECT_NE(source.find("projectionMatrix"), std::string::npos) << name;
  EXPECT_EQ(source.find("gbufferModelView"), std::string::npos) << name;
  EXPECT_EQ(source.find("gbufferProjection"), std::string::npos) << name;
 }
}
} // namespace net::minecraft::test
