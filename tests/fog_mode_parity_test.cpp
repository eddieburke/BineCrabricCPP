#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
namespace net::minecraft::test {
namespace {
std::string read(const std::filesystem::path& path) {
 std::ifstream input(path, std::ios::binary);
 return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
// P-FOGMODE: Java Iris uploads GL fog-mode constants (0 / GL_LINEAR / GL_EXP2)
// where the engine keeps internal 1/2/3. The FrameData producer transforms the
// internal mode before it reaches the shader.
client::render::PackUniformValues frameForFog(const client::render::core::FogUniforms& fog) {
 client::render::core::setFog(fog);
 client::render::FrameRenderCamera camera;
 client::render::FrameRenderCamera shadow;
 return client::render::buildShaderFrameData(
     1280, 720, 256.0f, 42.25f, 0, false, false, camera, shadow, nullptr);
}
}
TEST(FogModeParity, FrameDataProducerEmitsGlConstants) {
 constexpr int kGlLinear = 9729;
 constexpr int kGlExp2 = 2049;
 client::render::core::FogUniforms linear;
 linear.enabled = true;
 linear.mode = 1;
 EXPECT_EQ(frameForFog(linear).fogMode, kGlLinear);
 EXPECT_EQ(frameForFog(linear).fogShape, 1);
 client::render::core::FogUniforms exp2;
 exp2.enabled = true;
 exp2.mode = 3;
 EXPECT_EQ(frameForFog(exp2).fogMode, kGlExp2);
 client::render::core::FogUniforms exp;
 exp.enabled = true;
 exp.mode = 2;
 EXPECT_EQ(frameForFog(exp).fogMode, kGlExp2);
 client::render::core::FogUniforms off;
 EXPECT_EQ(frameForFog(off).fogMode, 0);
 EXPECT_EQ(frameForFog(off).fogShape, -1);
}
TEST(FogModeParity, RenderCoreProducerUsesSameMapping) {
 EXPECT_EQ(client::render::core::fogModeToGlConstant(1), 0x2601);
 EXPECT_EQ(client::render::core::fogModeToGlConstant(2), 0x0801);
 EXPECT_EQ(client::render::core::fogModeToGlConstant(3), 0x0801);
}
TEST(FogModeParity, CustomUniformsFogModeMatchesFrameValue) {
 std::vector<client::render::CustomUniformDecl> decls = {
     {"fogModeCopy", client::render::CustomUniformType::Int, true, "fogMode"}};
 client::render::CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 client::render::PackUniformValues frame;
 frame.fogMode = 9729;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 9729);
 frame.fogMode = 2049;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 2049);
 frame.fogMode = 0;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 0);
}
TEST(FogModeParity, VanillaCommonGlslDecodesGlConstants) {
 const std::filesystem::path shaders =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "vanilla" / "shaders";
 const std::string source = read(shaders / "lib" / "common.glsl");
  EXPECT_NE(source.find("fogMode == 9729"), std::string::npos);
  EXPECT_NE(source.find("fogMode == 2049"), std::string::npos);
  // Substring-safe: "fogMode == 2" would also match "fogMode == 2049".
  EXPECT_EQ(source.find("fogMode == 1)"), std::string::npos);
  EXPECT_EQ(source.find("fogMode == 2)"), std::string::npos);
  EXPECT_EQ(source.find("fogMode == 3)"), std::string::npos);
 EXPECT_EQ(source.find("as Iris reports them"), std::string::npos);
}
} // namespace net::minecraft::test
