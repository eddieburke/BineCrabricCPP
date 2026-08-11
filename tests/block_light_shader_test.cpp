#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/dimension/DimensionType.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackInstance;
using client::render::PackUniformValues;
using client::render::Tessellator;
class BlockLightShader : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "block-light-test", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  client::gl::GLCore::ensureLoaded();
 }
 static void TearDownTestSuite() {
  client::render::core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
 }
 static GLFWwindow* window_;
};
GLFWwindow* BlockLightShader::window_ = nullptr;
struct LightmapEntry {
 float luminance;
 std::uint8_t packed;
};
std::array<LightmapEntry, 16 * 16> generateLightmap(int ambient, const std::array<float, 16>& lumTable) {
 std::array<LightmapEntry, 16 * 16> out{};
 for(int sky = 0; sky < 16; ++sky) {
  for(int block = 0; block < 16; ++block) {
   const int effectiveSky = std::max(0, sky - ambient);
   const int level = std::clamp(std::max(block, effectiveSky), 0, 15);
   const float value = lumTable[static_cast<std::size_t>(level)];
   const std::uint8_t packed = static_cast<std::uint8_t>(std::lround(value * 255.0f));
   const std::size_t idx = static_cast<std::size_t>(sky * 16 + block);
   out[idx] = {value, packed};
  }
 }
 return out;
}
TEST_F(BlockLightShader, LightmapAtFullBlockLight) {
 DimensionType type;
 type.id = 0;
 type.brightnessFactor = 0.05f;
 type.isNether = false;
 Dimension dim(type);
 dim.initBrightnessTable();
 const auto lightmap = generateLightmap(0, dim.lightLevelToLuminance);
 for(int sky = 0; sky < 16; ++sky) {
  const std::size_t idx = static_cast<std::size_t>(sky * 16 + 15);
  const float value = lightmap[idx].luminance;
  EXPECT_GT(value, 0.9f) << "sky=" << sky << " should be near-full luminance at block=15";
  EXPECT_EQ(lightmap[idx].packed, 255) << "sky=" << sky << " packed byte should be 255 at block=15";
 }
 for(int block = 0; block < 16; ++block) {
  const std::size_t idx = static_cast<std::size_t>(0 * 16 + block);
  const float value = lightmap[idx].luminance;
  if(block >= 1) {
   EXPECT_GT(value, 0.0f) << "block=" << block << " should have nonzero luminance at sky=0";
  }
 }
}
TEST_F(BlockLightShader, LightmapTorchLightLevel) {
 DimensionType type;
 type.id = 0;
 type.brightnessFactor = 0.05f;
 type.isNether = false;
 Dimension dim(type);
 dim.initBrightnessTable();
 const auto lightmap = generateLightmap(0, dim.lightLevelToLuminance);
 const float torchLum14 = dim.lightLevelToLuminance[14];
 const float torchLum15 = dim.lightLevelToLuminance[15];
 EXPECT_GT(torchLum14, 0.7f) << "torch light level 14 should be bright";
 EXPECT_GT(torchLum15, 0.9f) << "max light level 15 should be near-full";
 EXPECT_GT(torchLum15, torchLum14) << "level 15 should be brighter than 14";
 const std::size_t idx14 = static_cast<std::size_t>(0 * 16 + 14);
 const std::size_t idx15 = static_cast<std::size_t>(0 * 16 + 15);
 EXPECT_GT(lightmap[idx15].luminance, lightmap[idx14].luminance)
     << "lightmap at block=15 should be brighter than block=14";
}
TEST_F(BlockLightShader, LightmapAmbientDarkness) {
 DimensionType type;
 type.id = 0;
 type.brightnessFactor = 0.05f;
 type.isNether = false;
 Dimension dim(type);
 dim.initBrightnessTable();
 const int ambient = 4;
 const auto noAmbient = generateLightmap(0, dim.lightLevelToLuminance);
 const auto withAmbient = generateLightmap(ambient, dim.lightLevelToLuminance);
 for(int sky = 0; sky < 16; ++sky) {
  for(int block = 0; block < 16; ++block) {
   const std::size_t idx = static_cast<std::size_t>(sky * 16 + block);
   const int effectiveSkyNoAmbient = std::max(0, sky);
   const int effectiveSkyWithAmbient = std::max(0, sky - ambient);
   const int levelNoAmbient = std::clamp(std::max(block, effectiveSkyNoAmbient), 0, 15);
   const int levelWithAmbient = std::clamp(std::max(block, effectiveSkyWithAmbient), 0, 15);
   if(levelWithAmbient < levelNoAmbient) {
    EXPECT_LT(withAmbient[idx].luminance, noAmbient[idx].luminance)
        << "block=" << block << " sky=" << sky << " ambient reduced light level";
   } else if(levelWithAmbient == levelNoAmbient) {
    EXPECT_FLOAT_EQ(withAmbient[idx].luminance, noAmbient[idx].luminance)
        << "block=" << block << " sky=" << sky << " ambient should not change light level";
   }
  }
 }
}
TEST_F(BlockLightShader, VertexLightPacking) {
 Tessellator tess;
 tess.startQuads();
 tess.light(14.0f, 0.0f);
 const std::int32_t expectedBlock = 14 * 16;
 const std::int32_t expectedSky = 0 * 16;
 const std::int32_t expected = expectedBlock | (expectedSky << 16);
 tess.vertex(0.0, 0.0, 0.0);
 auto mesh = tess.takeMesh();
 ASSERT_FALSE(mesh.empty());
 const auto& v = mesh.vertices[0];
 const std::int32_t block = v.light & 0xFFFF;
 const std::int32_t sky = (v.light >> 16) & 0xFFFF;
 EXPECT_EQ(block, expectedBlock) << "block light should be 14*16=224";
 EXPECT_EQ(sky, expectedSky) << "sky light should be 0";
 EXPECT_EQ(v.light, expected);
}
TEST_F(BlockLightShader, VertexLightPackingMaxValues) {
 Tessellator tess;
 tess.startQuads();
 tess.light(15.0f, 15.0f);
 tess.vertex(0.0, 0.0, 0.0);
 auto mesh = tess.takeMesh();
 ASSERT_FALSE(mesh.empty());
 const auto& v = mesh.vertices[0];
 const std::int32_t block = v.light & 0xFFFF;
 const std::int32_t sky = (v.light >> 16) & 0xFFFF;
 EXPECT_EQ(block, 240) << "max block light should be 15*16=240";
 EXPECT_EQ(sky, 240) << "max sky light should be 15*16=240";
}
TEST_F(BlockLightShader, VertexLightPackingOutOfRange) {
 Tessellator tess;
 tess.startQuads();
 tess.light(20.0f, -5.0f);
 tess.vertex(0.0, 0.0, 0.0);
 auto mesh = tess.takeMesh();
 ASSERT_FALSE(mesh.empty());
 const auto& v = mesh.vertices[0];
 const std::int32_t block = v.light & 0xFFFF;
 const std::int32_t sky = (v.light >> 16) & 0xFFFF;
 EXPECT_EQ(block, 240) << "block light should clamp to 15*16=240";
 EXPECT_EQ(sky, 0) << "sky light should clamp to 0";
}
TEST_F(BlockLightShader, BlockDataLightPacking) {
 Tessellator tess;
 tess.startQuads();
 tess.blockData(1.0, 2.0, 3.0, 14, 12, 8, 50, false, 0);
 tess.vertex(0.0, 0.0, 0.0);
 auto mesh = tess.takeMesh();
 ASSERT_FALSE(mesh.empty());
 const auto& v = mesh.vertices[0];
 const std::int32_t block = v.light & 0xFFFF;
 const std::int32_t sky = (v.light >> 16) & 0xFFFF;
 EXPECT_EQ(block, 12 * 16) << "block light from blockData should be 12*16=192";
 EXPECT_EQ(sky, 8 * 16) << "sky light from blockData should be 8*16=128";
}
TEST_F(BlockLightShader, LightmapPixelGenerationMatchesShaderExpectation) {
 DimensionType type;
 type.id = 0;
 type.brightnessFactor = 0.05f;
 type.isNether = false;
 Dimension dim(type);
 dim.initBrightnessTable();
 const auto lightmap = generateLightmap(0, dim.lightLevelToLuminance);
 for(int sky = 0; sky < 16; ++sky) {
  for(int block = 0; block < 16; ++block) {
   const std::size_t idx = static_cast<std::size_t>(sky * 16 + block);
   if(block == 15) {
    EXPECT_EQ(lightmap[idx].packed, 255)
        << "block=" << block << " sky=" << sky << " should be 255";
   }
   if(block == 0 && sky == 0) {
    EXPECT_LT(lightmap[idx].packed, 50)
        << "block=0 sky=0 should be near-zero luminance";
   }
   EXPECT_GE(lightmap[idx].luminance, 0.0f) << "luminance should never be negative";
   EXPECT_LE(lightmap[idx].luminance, 1.0f) << "luminance should never exceed 1.0";
  }
 }
}
TEST_F(BlockLightShader, UnifiedLightRegistryEmission) {
 world::light::UnifiedLightRegistry registry;
 const int torchId = 50;
 registry.setBlockEmission(torchId, 14);
 EXPECT_EQ(registry.blockEmission(torchId), 14);
 float r = 1.0f, g = 1.0f, b = 1.0f;
 registry.blockEmissionRGB(torchId, r, g, b);
 const float level = 14.0f / 15.0f;
 EXPECT_NEAR(r, level, 1e-5f);
 EXPECT_NEAR(g, level, 1e-5f);
 EXPECT_NEAR(b, level, 1e-5f);
 registry.setBlockLightColor(torchId, 1.0f, 0.8f, 0.5f);
 r = 1.0f;
 g = 1.0f;
 b = 1.0f;
 registry.blockEmissionRGB(torchId, r, g, b);
 EXPECT_NEAR(r, 1.0f * level, 1e-5f);
 EXPECT_NEAR(g, 0.8f * level, 2.0f / 255.0f);
 EXPECT_NEAR(b, 0.5f * level, 2.0f / 255.0f);
}
TEST_F(BlockLightShader, UnifiedLightRegistryOutOfBounds) {
 world::light::UnifiedLightRegistry registry;
 registry.setBlockEmission(-1, 14);
 registry.setBlockEmission(300, 14);
 EXPECT_EQ(registry.blockEmission(-1), 0);
 EXPECT_EQ(registry.blockEmission(300), 0);
 EXPECT_EQ(registry.blockEmission(0), 0);
}
TEST_F(BlockLightShader, LightmapCoordinateTransform) {
 const float blockLight = 14.0f;
 const float skyLight = 0.0f;
 const std::int32_t packed = (static_cast<std::int32_t>(blockLight) * 16) |
                             (static_cast<std::int32_t>(skyLight) * 16 << 16);
 float lmCoordX = (static_cast<float>(packed & 0xFFFF) / 256.0f) + (1.0f / 16.0f);
 float lmCoordY = (static_cast<float>((packed >> 16) & 0xFFFF) / 256.0f) + (1.0f / 16.0f);
 lmCoordX = std::clamp((lmCoordX - 0.03125f) * 1.06667f, 0.0f, 1.0f);
 lmCoordY = std::clamp((lmCoordY - 0.03125f) * 1.06667f, 0.0f, 1.0f);
 EXPECT_GT(lmCoordX, 0.0f) << "torch block light should map to nonzero lightmap U";
 EXPECT_LE(lmCoordX, 1.0f) << "torch block light lightmap U should clamp to 1.0";
 EXPECT_LT(lmCoordY, 0.1f) << "sky=0 should map to near-zero lightmap V";
 EXPECT_GT(lmCoordX, lmCoordY) << "block=14 should dominate sky=0 in lightmap coords";
}
TEST_F(BlockLightShader, LightmapCoordinateFullBright) {
 const float blockLight = 15.0f;
 const float skyLight = 15.0f;
 const std::int32_t packed = (static_cast<std::int32_t>(blockLight) * 16) |
                             (static_cast<std::int32_t>(skyLight) * 16 << 16);
 float lmCoordX = (static_cast<float>(packed & 0xFFFF) / 256.0f) + (1.0f / 16.0f);
 float lmCoordY = (static_cast<float>((packed >> 16) & 0xFFFF) / 256.0f) + (1.0f / 16.0f);
 lmCoordX = std::clamp((lmCoordX - 0.03125f) * 1.06667f, 0.0f, 1.0f);
 lmCoordY = std::clamp((lmCoordY - 0.03125f) * 1.06667f, 0.0f, 1.0f);
 EXPECT_FLOAT_EQ(lmCoordX, 1.0f) << "block=15 should clamp to 1.0";
 EXPECT_FLOAT_EQ(lmCoordY, 1.0f) << "sky=15 should clamp to 1.0";
}
}
} // namespace net::minecraft::test
