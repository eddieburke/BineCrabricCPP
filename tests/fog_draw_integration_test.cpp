#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
using client::gl::ShaderProgram;
using client::render::PackCompiler;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::PackUniformValues;
using client::render::VanillaPackEmbed;
namespace core = client::render::core;
using core::FogUniforms;
using core::RenderPass;
class FogDrawIntegrationTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
   ASSERT_EQ(glfwInit(), GLFW_TRUE);
   glfwDefaultWindowHints();
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    window_ = glfwCreateWindow(64, 64, "fog-draw-test", nullptr, nullptr);
    if(window_ == nullptr) {
     glfwDefaultWindowHints();
     glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
     window_ = glfwCreateWindow(64, 64, "fog-draw-test", nullptr, nullptr);
    }
    ASSERT_NE(window_, nullptr);
   glfwMakeContextCurrent(window_);
   GLCore::ensureLoaded();
   ASSERT_TRUE(GLCore::shaderSupported);
  }
  static void TearDownTestSuite() {
   core::releaseGlResources();
   glfwMakeContextCurrent(nullptr);
   if(window_ != nullptr) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
   }
  }
   void SetUp() override {
    if(window_ != nullptr) {
     glfwMakeContextCurrent(window_);
    }
    core::setActiveProgram(nullptr);
   }
   void TearDown() override {
    core::setActiveProgram(nullptr);
   }
   static GLFWwindow* window_;
 };
GLFWwindow* FogDrawIntegrationTest::window_ = nullptr;
FogUniforms worldFog() {
  FogUniforms fog;
  fog.enabled = true;
  fog.mode = 1;
  fog.shape = 0;
  fog.start = 192.0f;
  fog.end = 256.0f;
  fog.density = 0.0f;
  fog.color[0] = 0.5f;
  fog.color[1] = 0.6f;
  fog.color[2] = 0.8f;
  fog.color[3] = 1.0f;
  return fog;
}
bool loadVanillaPack(PackInstance& pack) {
 pack.embedded = true;
 const std::vector<std::string> resources = VanillaPackEmbed::resources();
 if(resources.empty()) {
  return false;
 }
 if(!PackLoader::load(resources,
                      [](std::string_view path) { return VanillaPackEmbed::get(path); },
                      pack.definition, pack.sourceOptions, pack.summary.error)) {
  return false;
 }
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 return true;
}
ShaderProgram* compileProgram(PackInstance& pack, const std::string& key) {
  if(pack.programs == nullptr) {
   pack.programs = std::make_unique<client::gl::ProgramCache>(std::filesystem::path{});
  }
  const auto log = [](PackInstance&, const std::string&, ::net::minecraft::util::logging::LogLevel) {};
  return PackCompiler::compile(pack, key, log);
}
void submitQuad(ShaderProgram* program) {
  core::setActiveProgram(program);
  RenderPass pass;
  pass.modelView = core::drawModelView();
  pass.projection = core::drawProjection();
  const float vertices[12] = {-1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f};
  pass.vertexData = vertices;
  pass.vertexCount = 4;
  pass.stride = 12;
  pass.glMode = 0x0004;
  core::submit(pass);
}
void* loadGlProc(const char* name) {
 return reinterpret_cast<void*>(glfwGetProcAddress(name));
}
using PFN_GetUniformiv = void(APIENTRY*)(GLuint, GLint, GLint*);
using PFN_GetUniformfv = void(APIENTRY*)(GLuint, GLint, GLfloat*);
int readIntUniform(ShaderProgram& program, const char* name) {
  const int loc = program.location(name);
  if(loc < 0) {
   return -1;
  }
  const auto getUniformiv = reinterpret_cast<PFN_GetUniformiv>(loadGlProc("glGetUniformiv"));
  int value = -9999;
  if(getUniformiv != nullptr) {
   getUniformiv(program.handle(), static_cast<GLint>(loc), &value);
  }
  return value;
}
float readFloatUniform(ShaderProgram& program, const char* name) {
  const int loc = program.location(name);
  if(loc < 0) {
   return -1.0f;
  }
  const auto getUniformfv = reinterpret_cast<PFN_GetUniformfv>(loadGlProc("glGetUniformfv"));
  float value = -9999.0f;
  if(getUniformfv != nullptr) {
   getUniformfv(program.handle(), static_cast<GLint>(loc), &value);
  }
  return value;
}
void readVec3Uniform(ShaderProgram& program, const char* name, float out[3]) {
  const int loc = program.location(name);
  if(loc < 0) {
   out[0] = out[1] = out[2] = 0.0f;
   return;
  }
  const auto getUniformfv = reinterpret_cast<PFN_GetUniformfv>(loadGlProc("glGetUniformfv"));
  if(getUniformfv != nullptr) {
   getUniformfv(program.handle(), static_cast<GLint>(loc), out);
  }
}
} // namespace
TEST_F(FogDrawIntegrationTest, WorldFogReachesTerrainAndEntityPrograms) {
  PackInstance pack;
  ASSERT_TRUE(loadVanillaPack(pack)) << "embedded vanilla pack failed to load";
  ASSERT_TRUE(pack.definition.programs.contains("gbuffers_terrain"));
  ASSERT_TRUE(pack.definition.programs.contains("gbuffers_entities"));
  ShaderProgram* terrain = compileProgram(pack, "gbuffers_terrain");
  ShaderProgram* entities = compileProgram(pack, "gbuffers_entities");
  ASSERT_NE(terrain, nullptr);
  ASSERT_NE(entities, nullptr);
  ASSERT_GE(terrain->location("fogMode"), 0);
  ASSERT_GE(entities->location("fogMode"), 0);
  core::setFog(worldFog());
  core::setFogEnabled(true);
  client::render::FrameRenderCamera camera;
  client::render::FrameRenderCamera shadow;
  PackUniformValues frame = client::render::buildShaderFrameData(
      64, 64, 0.0f, 0, false, false, camera, shadow, nullptr);
  EXPECT_EQ(frame.fogMode, 0x2601);
  client::render::uploadShaderUniforms(*terrain, frame, true);
  client::render::uploadShaderUniforms(*entities, frame, true);
  submitQuad(terrain);
  submitQuad(entities);
  const std::pair<ShaderProgram*, const char*> programs[] = {{terrain, "gbuffers_terrain"}, {entities, "gbuffers_entities"}};
  for(const auto& [program, name] : programs) {
   EXPECT_EQ(readIntUniform(*program, "fogMode"), 0x2601) << name;
   EXPECT_NEAR(readFloatUniform(*program, "fogStart"), 192.0f, 0.001f) << name;
   EXPECT_NEAR(readFloatUniform(*program, "fogEnd"), 256.0f, 0.001f) << name;
   float color[3]{};
   readVec3Uniform(*program, "fogColor", color);
   EXPECT_NEAR(color[0], 0.5f, 0.001f) << name;
   EXPECT_NEAR(color[1], 0.6f, 0.001f) << name;
   EXPECT_NEAR(color[2], 0.8f, 0.001f) << name;
  }
}
TEST_F(FogDrawIntegrationTest, SnapshotPushDoesNotClobberPerDrawFog) {
  PackInstance pack;
  ASSERT_TRUE(loadVanillaPack(pack));
  ShaderProgram* entities = compileProgram(pack, "gbuffers_entities");
  ASSERT_NE(entities, nullptr);
  core::setFog(worldFog());
  core::setFogEnabled(false);
  client::render::FrameRenderCamera camera;
  client::render::FrameRenderCamera shadow;
  PackUniformValues frame = client::render::buildShaderFrameData(
      64, 64, 0.0f, 0, false, false, camera, shadow, nullptr);
  client::render::uploadShaderUniforms(*entities, frame, true);
  submitQuad(entities);
  EXPECT_EQ(readIntUniform(*entities, "fogMode"), 0) << "interface draws must upload fogMode 0";
  core::setFogEnabled(true);
  submitQuad(entities);
  EXPECT_EQ(readIntUniform(*entities, "fogMode"), 0x2601)
      << "the per-draw fog must win over the frame snapshot after the uploader runs";
  EXPECT_NEAR(readFloatUniform(*entities, "fogStart"), 192.0f, 0.001f);
}
TEST_F(FogDrawIntegrationTest, ProgramFogClassGatesWorldFog) {
  PackInstance pack;
  ASSERT_TRUE(loadVanillaPack(pack));
  ShaderProgram* gui = compileProgram(pack, "gbuffers_gui");
  ShaderProgram* entities = compileProgram(pack, "gbuffers_entities");
  ASSERT_NE(gui, nullptr);
  ASSERT_NE(entities, nullptr);
  gui->setFogClass(false);
  core::setFog(worldFog());
  core::setFogEnabled(true);
  submitQuad(gui);
  EXPECT_LE(readIntUniform(*gui, "fogMode"), 0) << "interface program class must gate fog off";
  EXPECT_EQ(readIntUniform(*gui, "fogShape"), -1);
  submitQuad(entities);
  EXPECT_EQ(readIntUniform(*entities, "fogMode"), 0x2601) << "world program class keeps fog on";
  EXPECT_LE(readIntUniform(*entities, "fogShape"), 0);
  EXPECT_NEAR(readFloatUniform(*entities, "fogEnd"), 256.0f, 0.001f);
}
TEST_F(FogDrawIntegrationTest, ShadowProgramClassGatesFogOff) {
  PackInstance pack;
  ASSERT_TRUE(loadVanillaPack(pack));
  ShaderProgram* terrain = compileProgram(pack, "gbuffers_terrain");
  ASSERT_NE(terrain, nullptr);
  terrain->setFogClass(false);
  core::setFog(worldFog());
  core::setFogEnabled(true);
  submitQuad(terrain);
  EXPECT_EQ(readIntUniform(*terrain, "fogMode"), 0) << "shadow program class must gate fog off";
  EXPECT_EQ(readIntUniform(*terrain, "fogShape"), -1);
  terrain->setFogClass(true);
  submitQuad(terrain);
  EXPECT_EQ(readIntUniform(*terrain, "fogMode"), 0x2601);
}
} // namespace net::minecraft::test
