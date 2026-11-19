#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
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
    while(::glGetError() != GL_NO_ERROR) {}
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
   EXPECT_NEAR(readFloatUniform(*program, "betaFogStart"), 192.0f, 0.001f) << name;
   EXPECT_NEAR(readFloatUniform(*program, "betaFogEnd"), 256.0f, 0.001f) << name;
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
  EXPECT_NEAR(readFloatUniform(*entities, "betaFogStart"), 192.0f, 0.001f);
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
  EXPECT_NEAR(readFloatUniform(*entities, "betaFogEnd"), 256.0f, 0.001f);
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
TEST_F(FogDrawIntegrationTest, PipelineEnablesSkyFogOnlyForVanillaOutsideShadows) {
 PackInstance pack;
 ASSERT_TRUE(loadVanillaPack(pack));
 ShaderProgram* fallback = compileProgram(pack, "gbuffers_basic");
 ASSERT_NE(fallback, nullptr);
 using client::render::WorldProgramId;
 const std::array ids = {WorldProgramId::SkyBasic, WorldProgramId::SkyTextured,
                         WorldProgramId::TerrainSolid, WorldProgramId::Gui};
 for(const auto id : ids) {
  for(auto& runtime : pack.worldPrograms[static_cast<std::size_t>(id)]) {
   runtime.program = fallback;
   runtime.resolvedKey = "gbuffers_basic";
  }
 }
 client::render::Pipeline pipeline(nullptr);
 const auto savedCamera = core::cameraFrame();
 const auto savedFog = core::fog();
 const core::RenderStageScope stage(core::RenderStage::None);
 core::setFog(worldFog());
 for(const bool embedded : {false, true}) {
  pack.embedded = embedded;
  for(const bool shadow : {false, true}) {
   client::render::FrameRenderCamera camera;
   camera.shadowPass = shadow;
   core::setCameraFrame(camera);
   for(const auto id : ids) {
    SCOPED_TRACE(::testing::Message() << "embedded=" << embedded << " shadow=" << shadow
                                     << " id=" << static_cast<int>(id));
    ShaderProgram* program = pipeline.worldProgram(id, &pack);
    EXPECT_EQ(program, fallback);
    submitQuad(program);
    const bool expected = !shadow && (id == WorldProgramId::TerrainSolid ||
                                       (embedded && id == WorldProgramId::SkyBasic));
    EXPECT_EQ(readIntUniform(*program, "fogMode"), expected ? 0x2601 : 0);
   }
  }
 }
 core::setFog(savedFog);
 core::setCameraFrame(savedCamera);
}
TEST_F(FogDrawIntegrationTest, VanillaSkyBlendsAtHorizonWithoutFoggingStarsOrSunset) {
 PackInstance pack;
 ASSERT_TRUE(loadVanillaPack(pack));
 ShaderProgram* sky = compileProgram(pack, "gbuffers_skybasic");
 ASSERT_NE(sky, nullptr);
 using client::render::WorldProgramId;
 pack.worldPrograms[static_cast<std::size_t>(WorldProgramId::SkyBasic)][0].program = sky;
 client::render::Pipeline pipeline(nullptr);
 const auto savedCamera = core::cameraFrame();
 const auto savedFog = core::fog();
 const core::ScopedDrawCameraState drawCamera;
 const core::RenderStageScope stage(core::RenderStage::None);
 const core::DepthScope depth(false, false);
 const core::BlendScope blend(false);
 const core::CullScope cull(false);
 client::render::FrameRenderCamera camera;
 camera.viewForwardZ = -1.0f;
 core::setCameraFrame(camera);
 core::setDrawCameraStateFromCamera(camera);
 GLCore::bindFramebuffer(GL_FRAMEBUFFER, 0);
 core::viewport(0, 0, 64, 64);
 ::glReadBuffer(GL_BACK);
 FogUniforms fog = worldFog();
 fog.start = 0.0f;
 fog.end = 204.8f;
 core::setFog(fog);
 core::setActiveProgram(pipeline.worldProgram(WorldProgramId::SkyBasic, &pack));
 const float skyColor[3] = {51.0f / 255.0f, 102.0f / 255.0f, 204.0f / 255.0f};
 for(const auto renderStage : {core::RenderStage::Sky, core::RenderStage::Void,
                               core::RenderStage::Stars, core::RenderStage::Sunset}) {
  core::setRenderStage(renderStage);
  ::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  ::glClear(GL_COLOR_BUFFER_BIT);
  auto& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  tessellator.color(skyColor[0], skyColor[1], skyColor[2], 1.0f);
  tessellator.vertex(-352.0, 16.0, -352.0);
  tessellator.vertex(352.0, 16.0, -352.0);
  tessellator.vertex(352.0, 16.0, 352.0);
  tessellator.vertex(-352.0, 16.0, 352.0);
  tessellator.draw();
  for(const int y : {34, 48}) {
   SCOPED_TRACE(::testing::Message() << "stage=" << static_cast<int>(renderStage) << " y=" << y);
   float pixel[4]{};
   ::glReadPixels(32, y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
   const float viewDistance = 16.0f / ((static_cast<float>(y) + 0.5f) / 32.0f - 1.0f);
   const bool fogged = renderStage == core::RenderStage::Sky || renderStage == core::RenderStage::Void;
   const float factor = fogged ? std::clamp((fog.end - viewDistance) / fog.end, 0.0f, 1.0f) : 1.0f;
   for(int channel = 0; channel < 3; ++channel) {
    EXPECT_NEAR(pixel[channel], fog.color[channel] * (1.0f - factor) + skyColor[channel] * factor,
                2.0f / 255.0f);
   }
   EXPECT_NEAR(pixel[3], 1.0f, 1.0f / 255.0f);
  }
 }
 EXPECT_EQ(::glGetError(), GL_NO_ERROR);
 core::setFog(savedFog);
 core::setCameraFrame(savedCamera);
}
TEST_F(FogDrawIntegrationTest, CustomFogSettingsReachWorldAndResetWhenDisabled) {
 class FogMinecraft final : public client::Minecraft {
  public:
  using client::Minecraft::Minecraft;
  void handleCrash(const util::crash::CrashReport&) override {}
 };
 struct RestoreState {
  client::Minecraft* instance = client::Minecraft::INSTANCE;
  FogUniforms fog = core::fog();
  std::vector<mod::ModSettingDef> settings = mod::ModSettingsRegistry::instance().getSettings("fog_settings");
  ~RestoreState() {
   client::Minecraft::INSTANCE = instance;
   core::setFog(fog);
   auto& registry = mod::ModSettingsRegistry::instance();
   for(const auto& setting : settings) {
    *registry.findSetting("fog_settings", setting.key) = setting;
   }
  }
 } restore;
 FogMinecraft minecraft(64, 64, false);
 minecraft.options.modsEnabled = true;
 minecraft.options.shaderPack = "vanilla";
 World world("fog_settings_test", 42);
 entity::player::PlayerEntity camera(&world);
 camera.setPosition(0.0, 80.0, 0.0);
 minecraft.world = &world;
 minecraft.camera = &camera;
 auto& registry = mod::ModSettingsRegistry::instance();
 ASSERT_TRUE(mod::runtime::hasLuaHook(mod::runtime::LuaEventId::FogSettings));
 const auto toggle = [&](const char* key, bool value) {
  auto* setting = registry.findSetting("fog_settings", key);
  ASSERT_NE(setting, nullptr) << key;
  setting->boolCurrent = value;
 };
 const auto slider = [&](const char* key, float value) {
  auto* setting = registry.findSetting("fog_settings", key);
  ASSERT_NE(setting, nullptr) << key;
  setting->floatCurrent = value;
 };
 toggle("enabled", true);
 toggle("spherical", true);
 toggle("exponential", false);
 toggle("custom_color", true);
 slider("start", 0.25f);
 slider("end", 0.75f);
 slider("density", 0.15f);
 slider("red", 0.1f);
 slider("green", 0.3f);
 slider("blue", 0.7f);
 client::option::RenderSettings frame;
 core::fogUpdateFromWorld(&minecraft, 0.0f, frame);
 core::fogApplyMode(&minecraft, false, frame);
 EXPECT_TRUE(core::fog().custom);
 EXPECT_EQ(core::fog().shape, 0);
 EXPECT_FLOAT_EQ(core::fog().start, frame.renderDistance.fogEnd() * 0.25f);
 EXPECT_FLOAT_EQ(core::fog().end, frame.renderDistance.fogEnd() * 0.75f);
 EXPECT_FLOAT_EQ(core::fog().environmentalStart, core::fog().start);
 EXPECT_FLOAT_EQ(core::fog().environmentalEnd, core::fog().end);
 EXPECT_FLOAT_EQ(core::fog().color[0], 0.1f);
 EXPECT_FLOAT_EQ(core::fog().color[1], 0.3f);
 EXPECT_FLOAT_EQ(core::fog().color[2], 0.7f);
 toggle("spherical", false);
 toggle("exponential", true);
 core::fogUpdateFromWorld(&minecraft, 0.0f, frame);
 core::fogApplyMode(&minecraft, false, frame);
 EXPECT_EQ(core::fog().shape, 1);
 EXPECT_EQ(core::fog().mode, 2);
 EXPECT_FLOAT_EQ(core::fog().density, 0.15f);
 minecraft.options.modsEnabled = false;
 core::fogUpdateFromWorld(&minecraft, 0.0f, frame);
 core::fogApplyMode(&minecraft, false, frame);
 EXPECT_FALSE(core::fog().custom);
 EXPECT_EQ(core::fog().mode, 1);
 EXPECT_FLOAT_EQ(core::fog().end, frame.renderDistance.fogEnd());
 EXPECT_FLOAT_EQ(core::fog().start, frame.renderDistance.fogStart());
 EXPECT_FLOAT_EQ(core::fog().environmentalStart, frame.renderDistance.fogEnd());
 EXPECT_FLOAT_EQ(core::fog().environmentalEnd, frame.renderDistance.farPlane());
 PackUniformValues packFrame = client::render::buildShaderFrameData(
     64, 64, 0.0f, 0, false, false, client::render::FrameRenderCamera{}, client::render::FrameRenderCamera{}, nullptr);
 EXPECT_FLOAT_EQ(packFrame.fogStart, frame.renderDistance.fogEnd());
 EXPECT_FLOAT_EQ(packFrame.fogEnd, frame.renderDistance.farPlane());
 minecraft.world = nullptr;
 minecraft.camera = nullptr;
}
TEST_F(FogDrawIntegrationTest, CustomProjectionChangesFogCoordinateAndRestoresPlanarDefault) {
 ShaderProgram program;
 const std::string common = VanillaPackEmbed::get("shaders/lib/common.glsl");
 ASSERT_FALSE(common.empty());
 ASSERT_TRUE(program.compile(
     "layout(location=0) in vec3 position;void main(){gl_Position=vec4(position,1.0);}",
     common + "\nlayout(location=0) out vec4 color;void main(){color=vec4(fogCoord(vec3(3,4,-12))/20.0);}",
     "#version 430 core\n"));
 const core::DepthScope depth(false, false);
 const core::BlendScope blend(false);
 const core::CullScope cull(false);
 GLCore::bindFramebuffer(GL_FRAMEBUFFER, 0);
 core::viewport(0, 0, 64, 64);
 ::glReadBuffer(GL_BACK);
 const auto savedFog = core::fog();
 for(const bool custom : {true, false}) {
  for(const int shape : {0, 1}) {
   FogUniforms fog = worldFog();
   fog.custom = custom;
   fog.shape = shape;
   core::setFog(fog);
   submitQuad(&program);
   float pixel[4]{};
   ::glReadPixels(40, 20, 1, 1, GL_RGBA, GL_FLOAT, pixel);
   const float expected = !custom ? 12.0f : shape == 0 ? 13.0f : std::sqrt(153.0f);
   EXPECT_NEAR(pixel[0], expected / 20.0f, 2.0f / 255.0f);
  }
 }
 core::setActiveProgram(nullptr);
 core::setFog(savedFog);
}
TEST(BetaVisualStyle, SkyColorsMatchJavaHsbRounding) {
 const std::array<float, 8> temperatures{-6.0f, -3.0f, 0.0f, 0.5f, 0.8f, 1.0f, 3.0f, 6.0f};
 const std::array<int, 8> colors{0x9C99FF, 0x9C99FF, 0x80A2FF, 0x7BA5FF,
                                0x79A7FF, 0x77A9FF, 0x66BDFF, 0x66BDFF};
 for(std::size_t i = 0; i < temperatures.size(); ++i) {
  EXPECT_EQ(Biome::plains().getSkyColor(temperatures[i]), colors[i]) << temperatures[i];
 }
}
TEST_F(FogDrawIntegrationTest, VanillaLightmapMatchesJavaBrightnessWithoutChangingShaderpacks) {
 const std::array<std::array<int, 16>, 6> expected{{
     {13, 17, 22, 27, 33, 40, 47, 56, 67, 79, 94, 111, 134, 163, 201, 255},
     {13, 19, 27, 35, 44, 53, 64, 76, 89, 104, 120, 140, 162, 188, 218, 255},
     {13, 29, 45, 61, 77, 94, 110, 126, 142, 158, 174, 190, 207, 223, 239, 255},
     {26, 30, 34, 39, 45, 51, 58, 67, 77, 88, 102, 119, 140, 168, 204, 255},
     {45, 50, 57, 64, 71, 80, 89, 99, 111, 124, 138, 155, 174, 197, 223, 255},
     {64, 77, 89, 102, 115, 128, 140, 153, 166, 179, 191, 204, 217, 230, 242, 255}}};
 client::option::GameOptions options;
 client::render::Pipeline pipeline(&options);
 World world("beta_visual_style", 42);
 const int savedUnit = core::getActiveTextureUnit();
 for(int dimension = 0; dimension < 2; ++dimension) {
  world.dimension = Dimension::fromId(dimension == 0 ? 0 : -1);
  world.dimension->initBrightnessTable();
  for(const char* selection : {"vanilla", "off", "rethinking-voxels"}) {
   options.shaderPack = selection;
   for(const int ambient : {0, 11}) {
    world.ambientDarkness = ambient;
    for(const int step : {0, 2, 1, 0}) {
     options.brightness = static_cast<float>(step) * 0.5f;
     pipeline.refreshLightmap(&world);
     ASSERT_NE(pipeline.lightmapTexture(), 0u);
     core::activeTexture(GL_TEXTURE0);
     core::bindTexture(pipeline.lightmapTexture());
     std::array<unsigned char, 16 * 16 * 4> pixels{};
     ::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
     const int curve = dimension * 3 + (options.shaderPack == "rethinking-voxels" ? 0 : step);
     for(int sky = 0; sky < 16; ++sky) {
      for(int block = 0; block < 16; ++block) {
       const int level = std::max(block, std::max(0, sky - ambient));
       const int offset = (sky * 16 + block) * 4;
       for(int channel = 0; channel < 3; ++channel) {
        EXPECT_EQ(pixels[offset + channel], expected[curve][level])
            << selection << " dimension=" << dimension << " brightness=" << options.brightness
            << " ambient=" << ambient << " sky=" << sky << " block=" << block;
       }
       EXPECT_EQ(pixels[offset + 3], 255);
      }
     }
    }
   }
  }
 }
 core::activeTexture(GL_TEXTURE0 + savedUnit);
 EXPECT_EQ(::glGetError(), GL_NO_ERROR);
}
TEST_F(FogDrawIntegrationTest, VanillaInterpolatesVertexBrightnessBeforeTextureShading) {
 PackInstance pack;
 ASSERT_TRUE(loadVanillaPack(pack));
 client::option::GameOptions options;
 options.shaderPack = "vanilla";
 client::render::Pipeline pipeline(&options);
 World world("beta_vertex_light", 42);
 world.ambientDarkness = 0;
 pipeline.refreshLightmap(&world);
 const auto savedCamera = core::cameraFrame();
 const auto savedFog = core::fog();
 const bool savedLighting = core::lightingEnabled();
 const core::ScopedDrawCameraState drawCamera;
 const core::DepthScope depth(false, false);
 const core::BlendScope blend(false);
 const core::CullScope cull(false);
 client::render::FrameRenderCamera camera;
 camera.orthographic = true;
 camera.viewForwardZ = -1.0f;
 core::setCameraFrame(camera);
 core::setDrawCameraStateFromCamera(camera);
 core::setFogEnabled(false);
 core::setLightingEnabled(false);
 GLCore::bindFramebuffer(GL_FRAMEBUFFER, 0);
 core::viewport(0, 0, 64, 64);
 ::glReadBuffer(GL_BACK);
 client::gl::GlTexture albedo(core::genTexture());
 core::activeTexture(GL_TEXTURE0);
 core::bindTexture(albedo.handle());
 const std::array<unsigned char, 4> texel{64, 128, 192, 255};
 ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel.data());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 core::activeTexture(GL_TEXTURE1);
 core::bindTexture(pipeline.lightmapTexture());
 core::activeTexture(GL_TEXTURE0);
 for(const char* key : {"gbuffers_terrain", "gbuffers_water", "gbuffers_basic",
                         "gbuffers_textured_lit", "gbuffers_entities", "gbuffers_hand"}) {
  ShaderProgram* program = compileProgram(pack, key);
  ASSERT_NE(program, nullptr) << key;
  program->bind();
  program->set1i("gtexture", 0);
  program->set1i("lightmap", 1);
  core::setActiveProgram(program);
  auto& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  tessellator.color(1.0f, 1.0f, 1.0f, 1.0f);
  tessellator.normal(0.0f, 1.0f, 0.0f);
  tessellator.light(0, 0);
  tessellator.vertex(-1.0, -1.0, -1.0, 0.0, 0.0);
  tessellator.light(15, 0);
  tessellator.vertex(1.0, -1.0, -1.0, 1.0, 0.0);
  tessellator.vertex(1.0, 1.0, -1.0, 1.0, 1.0);
  tessellator.light(0, 0);
  tessellator.vertex(-1.0, 1.0, -1.0, 0.0, 1.0);
  tessellator.draw();
  for(const int x : {8, 24, 40, 56}) {
   std::array<unsigned char, 4> pixel{};
   ::glReadPixels(x, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
   const float light = 13.0f / 255.0f + (1.0f - 13.0f / 255.0f) * (x + 0.5f) / 64.0f;
   for(int channel = 0; channel < 3; ++channel) {
    EXPECT_NEAR(pixel[channel], texel[channel] * light, 2.0f) << key << " x=" << x;
   }
  }
 }
 core::setActiveProgram(nullptr);
 core::setLightingEnabled(savedLighting);
 core::setFog(savedFog);
 core::setCameraFrame(savedCamera);
 EXPECT_EQ(::glGetError(), GL_NO_ERROR);
}
TEST_F(FogDrawIntegrationTest, VanillaFinalPreservesSkyFogAndTexturePaletteAtEveryBrightness) {
 PackInstance pack;
 ASSERT_TRUE(loadVanillaPack(pack));
 ShaderProgram* program = compileProgram(pack, "final");
 ASSERT_NE(program, nullptr);
 const core::DepthScope depth(false, false);
 const core::BlendScope blend(false);
 const core::CullScope cull(false);
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 targets.bindGbuffers();
 core::viewport(0, 0, 4, 4);
 ::glReadBuffer(GL_COLOR_ATTACHMENT0);
 client::gl::GlTexture scene(core::genTexture());
 core::activeTexture(GL_TEXTURE0);
 core::bindTexture(scene.handle());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 program->bind();
 program->set1i("colortex0", 0);
 for(const std::array<unsigned char, 4> color : {
         std::array<unsigned char, 4>{121, 167, 255, 255}, {192, 216, 255, 255},
         {16, 48, 96, 128}, {64, 128, 192, 255}}) {
  ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color.data());
  for(const float brightness : {0.0f, 0.5f, 1.0f}) {
   program->set1f("screenBrightness", brightness);
   core::drawFullscreen();
   std::array<unsigned char, 4> pixel{};
   ::glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
   EXPECT_EQ(pixel, color) << "brightness=" << brightness;
  }
 }
 ShaderProgram::unbind();
 targets.endGbuffers();
 EXPECT_EQ(::glGetError(), GL_NO_ERROR);
}
} // namespace net::minecraft::test
