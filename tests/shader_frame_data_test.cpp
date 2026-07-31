#include <gtest/gtest.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderFrameData.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
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
}
TEST(ShaderUniformDefaults, MatricesAreInvertibleBeforeFirstWorldFrame) {
 const client::render::shaderpack::ShaderUniformValues values;
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
 const auto first = client::render::shaderpack::buildShaderFrameData(
     1280, 720, 256.0f, 42.25f, 0, false, false, camera, shadow, nullptr);
 camera.projectionX = 1.5f;
 const auto second = client::render::shaderpack::buildShaderFrameData(
     1280, 720, 256.0f, 42.25f, 0, false, false, camera, shadow, nullptr);
 EXPECT_EQ(second.frameCounter, (first.frameCounter + 1) % 720720);
 EXPECT_GE(second.frameTime, 0.0f);
 EXPECT_GE(second.frameTimeCounter, first.frameTimeCounter);
 EXPECT_TRUE(std::equal(std::begin(first.gbufferProjection), std::end(first.gbufferProjection),
                        std::begin(second.gbufferPreviousProjection)));
}
TEST(ShaderFrameData, WrapsCameraPositionAndKeepsExactSplit) {
 client::render::FrameRenderCamera camera;
 client::render::FrameRenderCamera shadow;
 camera.eyeX = 30001.25;
 camera.eyeY = -30001.75;
 camera.eyeZ = 60002.5;
 const auto values = client::render::shaderpack::buildShaderFrameData(
     1280, 720, 256.0f, 0.0f, 0, false, false, camera, shadow, nullptr);
 EXPECT_FLOAT_EQ(values.cameraPosition[0], 1.25f);
 EXPECT_FLOAT_EQ(values.cameraPosition[1], -1.75f);
 EXPECT_FLOAT_EQ(values.cameraPosition[2], 2.5f);
 EXPECT_EQ(values.cameraPositionInt[0], 30001);
 EXPECT_EQ(values.cameraPositionInt[1], -30002);
 EXPECT_EQ(values.cameraPositionInt[2], 60002);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[0], 0.25f);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[1], 0.25f);
 EXPECT_FLOAT_EQ(values.cameraPositionFract[2], 0.5f);
}
TEST(VanillaShaderAbi, GeometryUsesPerDrawCoreMatrices) {
 const std::filesystem::path shaders =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaderpacks" / "vanilla" / "shaders";
 const std::vector<std::string> names = {
     "gbuffers_basic.vsh",          "gbuffers_entities.vsh", "gbuffers_gui.vsh",
     "gbuffers_gui_textured.vsh",   "gbuffers_hand.vsh",     "gbuffers_item.vsh",
     "gbuffers_line.vsh",           "gbuffers_skybasic.vsh", "gbuffers_skytextured.vsh",
     "gbuffers_terrain.vsh",        "gbuffers_text.vsh",     "gbuffers_textured.vsh",
     "gbuffers_textured_lit.vsh",   "gbuffers_water.vsh"};
 for(const std::string& name : names) {
  const std::string source = read(shaders / name);
  EXPECT_FALSE(source.empty()) << name;
  EXPECT_NE(source.find("modelViewMatrix"), std::string::npos) << name;
  EXPECT_NE(source.find("projectionMatrix"), std::string::npos) << name;
  EXPECT_EQ(source.find("gbufferModelView"), std::string::npos) << name;
  EXPECT_EQ(source.find("gbufferProjection"), std::string::npos) << name;
 }
}
}
