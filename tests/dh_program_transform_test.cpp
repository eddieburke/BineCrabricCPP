#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
using client::render::PackCompiler;
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::ShaderStage;
using client::render::VanillaPackEmbed;
namespace core = client::render::core;
std::filesystem::path packRoot(std::string_view name) {
 return std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / name;
}
std::string prepared(const std::filesystem::path& root, const std::string& path, const std::string& program,
                     ShaderStage stage, const PackDefinition& definition) {
 std::unordered_map<std::string, std::string> memo;
 const std::string included = client::render::resolveShaderIncludes(
     [&root](std::string_view file) {
      std::ifstream stream(root / std::filesystem::path(file), std::ios::binary);
      std::ostringstream buffer;
      buffer << stream.rdbuf();
      return buffer.str();
     },
     path, false, memo);
 return client::render::prepareSource(program, stage, definition, included);
}
std::string legacyIdentifiers(const std::string& source, bool vertex) {
 const std::regex legacy(vertex ? R"(\b(gl_(Vertex|Color|Normal|MultiTexCoord\d|ModelView\w*|Projection\w*|NormalMatrix|TextureMatrix)|ftransform)\b)"
                                : R"(\bgl_(ModelView\w*|Projection\w*|NormalMatrix)\b)");
 std::smatch match;
 return std::regex_search(source, match, legacy) ? match.str() : std::string{};
}
std::string vanillaFragment(const std::string& program, const PackDefinition& definition) {
 std::unordered_map<std::string, std::string> memo;
 return client::render::prepareSource(program, ShaderStage::Fragment, definition,
     client::render::resolveShaderIncludes([](std::string_view path) { return VanillaPackEmbed::get(path); },
                                          "shaders/" + program + ".fsh", false, memo));
}
const std::string surfaceVertex =
    "out vec2 texcoord;out vec3 vertexLight;out vec4 color;out vec3 normal;out float viewDistance;"
    "out vec2 horizontalPosition;out vec3 nativeViewPosition;out float directionalShade;"
    "uniform float testDistance;uniform vec4 testColor;uniform vec3 testLight;uniform float testShade;"
    "void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));"
    "gl_Position=vec4(p*2.0-1.0,0.0,1.0);texcoord=vec2(0.5);vertexLight=testLight;color=testColor;"
    "normal=vec3(0.0,1.0,0.0);viewDistance=testDistance;horizontalPosition=vec2(testDistance,0.0);"
    "nativeViewPosition=vec3(0.0,0.0,-testDistance);directionalShade=testShade;}";
void expectDhProgram(const std::filesystem::path& root, const std::string& directory, const std::string& program) {
 PackDefinition definition;
 definition.distantHorizons = true;
 const std::string base = "shaders/" + directory + "/" + program;
 ASSERT_TRUE(std::filesystem::exists(root / (base + ".vsh"))) << base;
 const std::string vertex = prepared(root, base + ".vsh", program, ShaderStage::Vertex, definition);
 const std::string fragment = prepared(root, base + ".fsh", program, ShaderStage::Fragment, definition);
 EXPECT_EQ(legacyIdentifiers(vertex, true), "") << base << ".vsh";
 EXPECT_EQ(legacyIdentifiers(fragment, false), "") << base << ".fsh";
 EXPECT_NE(vertex.find("_vert_init"), std::string::npos) << base;
 EXPECT_NE(vertex.find("_vert_init();"), std::string::npos) << base;
 EXPECT_NE(vertex.find("in uvec4 vPosition"), std::string::npos) << base;
 EXPECT_NE(vertex.find("uniform vec3 modelOffset"), std::string::npos) << base;
 EXPECT_EQ(vertex.find("vaPosition"), std::string::npos) << base;
}
class DhProgramTransform : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "dh-program-test", nullptr, nullptr);
  if(window_ == nullptr) {
   glfwDefaultWindowHints();
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   window_ = glfwCreateWindow(64, 64, "dh-program-test", nullptr, nullptr);
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
  if(window_ != nullptr) glfwMakeContextCurrent(window_);
  core::setActiveProgram(nullptr);
  core::setCameraFrame({});
 }
 static GLFWwindow* window_;
};
GLFWwindow* DhProgramTransform::window_ = nullptr;
}
TEST_F(DhProgramTransform, ProgramIdsRegistered) {
 bool terrain = false;
 bool water = false;
 bool shadow = false;
 for(const client::render::PackProgramId& id : client::render::packProgramIds()) {
  if(id.name == "dh_terrain") terrain = id.fallback.empty();
  if(id.name == "dh_water") water = id.fallback == "dh_terrain";
  if(id.name == "dh_shadow") shadow = id.fallback.empty();
 }
 EXPECT_TRUE(terrain);
 EXPECT_TRUE(water);
 EXPECT_TRUE(shadow);
}
TEST_F(DhProgramTransform, DistantHorizonsMacroFollowsPackFlag) {
 PackDefinition pack;
 const auto has = [](const PackDefinition& definition) {
  for(const auto& macro : client::render::engineMacros(definition)) {
   if(macro.name == "DISTANT_HORIZONS") return true;
  }
  return false;
 };
 EXPECT_FALSE(has(pack));
 pack.distantHorizons = true;
 EXPECT_TRUE(has(pack));
 const std::string source = "#ifdef DISTANT_HORIZONS\nint on;\n#else\nint off;\n#endif\n";
 EXPECT_NE(client::render::normalizePackSource(pack, source).find("int on;"), std::string::npos);
 pack.distantHorizons = false;
 EXPECT_NE(client::render::normalizePackSource(pack, source).find("int off;"), std::string::npos);
}
TEST_F(DhProgramTransform, SyntheticVertexLowering) {
 PackDefinition pack;
 const std::string source =
     "#version 330\n"
     "out vec2 uv;\n"
     "void main() {\n"
     " gl_Position = ftransform();\n"
     " vec4 p = gl_ModelViewMatrix * gl_Vertex;\n"
     " uv = gl_MultiTexCoord1.xy + gl_MultiTexCoord0.xy + (gl_TextureMatrix[0] * gl_MultiTexCoord2).xy;\n"
     " vec3 n = gl_NormalMatrix * gl_Normal + gl_Color.rgb + (gl_ProjectionMatrixInverse * p).xyz;\n"
     "}\n";
 const std::string result = client::render::prepareSource("dh_terrain", ShaderStage::Vertex, pack, source);
 EXPECT_EQ(legacyIdentifiers(result, true), "");
 EXPECT_NE(result.find("(iris_ProjectionMatrix * iris_ModelViewMatrix * vec4(modelOffset + _vert_position, 1.0))"),
           std::string::npos);
 EXPECT_NE(result.find("iris_ModelViewMatrix * vec4(modelOffset + _vert_position, 1.0)"), std::string::npos);
 EXPECT_NE(result.find("mat4(1.0)"), std::string::npos);
 EXPECT_NE(result.find("vec4(_vert_tex_light_coord, 0.0, 1.0)"), std::string::npos);
 EXPECT_NE(result.find("uniform mat3 iris_NormalMatrix;"), std::string::npos);
 EXPECT_NE(result.find("uniform mat4 iris_ProjectionMatrixInverse;"), std::string::npos);
 EXPECT_NE(result.find("in vec4 iris_color;"), std::string::npos);
 EXPECT_NE(result.find("in uvec4 irisExtra;"), std::string::npos);
 EXPECT_NE(result.find("uniform float mircoOffset;"), std::string::npos);
 const std::size_t init = result.find("_vert_init();");
 const std::size_t mainAt = result.find("void main()");
 ASSERT_NE(init, std::string::npos);
 EXPECT_GT(init, mainAt);
}
TEST_F(DhProgramTransform, ComplementaryReimaginedPrograms) {
 const std::filesystem::path root = packRoot("ComplementaryReimagined_r5.8.1");
 ASSERT_TRUE(std::filesystem::is_directory(root));
 expectDhProgram(root, "world0", "dh_terrain");
 expectDhProgram(root, "world0", "dh_water");
}
TEST_F(DhProgramTransform, RenderPearlPrograms) {
 const std::filesystem::path root = packRoot("RenderPearl v2.8.0-beta.4");
 ASSERT_TRUE(std::filesystem::is_directory(root));
 expectDhProgram(root, "world_default", "dh_terrain");
 expectDhProgram(root, "world_default", "dh_water");
 expectDhProgram(root, "world_default", "dh_shadow");
}
TEST_F(DhProgramTransform, VanillaEmbeddedProgramsCompile) {
 for(const bool distantHorizons : {false, true}) {
  PackInstance pack;
  pack.embedded = true;
  const std::vector<std::string> resources = VanillaPackEmbed::resources();
  ASSERT_FALSE(resources.empty());
  ASSERT_TRUE(PackLoader::load(resources, [](std::string_view path) { return VanillaPackEmbed::get(path); },
                               pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
  pack.definition.distantHorizons = distantHorizons;
  pack.summary.valid = true;
  pack.rootDefinition = pack.definition;
  pack.programs = std::make_unique<client::gl::ProgramCache>(std::filesystem::path{});
  ASSERT_TRUE(pack.definition.programs.contains("dh_terrain"));
  ASSERT_TRUE(pack.definition.programs.contains("dh_water"));
  std::string failure;
  const auto log = [&failure](PackInstance&, const std::string& message, ::net::minecraft::util::logging::LogLevel) {
   failure += message + "\n";
  };
  if(distantHorizons) {
   EXPECT_NE(PackCompiler::compile(pack, "dh_terrain", log), nullptr) << failure;
   EXPECT_NE(PackCompiler::compile(pack, "dh_water", log), nullptr) << failure;
  } else {
   EXPECT_EQ(PackCompiler::compile(pack, "dh_terrain", log), nullptr);
   EXPECT_EQ(PackCompiler::compile(pack, "dh_water", log), nullptr);
   EXPECT_TRUE(pack.compiledPrograms.empty());
   EXPECT_TRUE(pack.preparedSourceCache.empty());
  }
  EXPECT_NE(PackCompiler::compile(pack, "gbuffers_terrain", log), nullptr) << failure;
  EXPECT_NE(PackCompiler::compile(pack, "gbuffers_water", log), nullptr) << failure;
  EXPECT_TRUE(failure.empty()) << failure;
 }
}
TEST_F(DhProgramTransform, VanillaWaterUsesNativeProjectionToRejectForegroundTerrain) {
 PackDefinition definition;
 definition.distantHorizons = true;
 std::unordered_map<std::string, std::string> memo;
 const std::string fragment = client::render::prepareSource("dh_water", ShaderStage::Fragment, definition,
     client::render::resolveShaderIncludes([](std::string_view path) { return VanillaPackEmbed::get(path); },
                                          "shaders/dh_water.fsh", false, memo));
 const std::string vertex =
     "out vec3 vertexLight;out vec4 color;out float viewDistance;out vec2 horizontalPosition;"
     "out vec3 nativeViewPosition;out float directionalShade;void main(){"
     "vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));"
     "gl_Position=vec4(p*2.0-1.0,0.0,1.0);vertexLight=vec3(1.0);"
     "color=vec4(0.0,0.0,1.0,0.6);viewDistance=500.0;"
     "horizontalPosition=vec2(500.0,0.0);nativeViewPosition=vec3(0.0,0.0,-500.0);directionalShade=1.0;}";
 client::gl::ShaderProgram program;
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 client::gl::GlTexture lightmap(core::genTexture());
 client::gl::GlTexture nativeDepth(core::genTexture());
 core::activeTexture(0x84C0);
 core::bindTexture(lightmap.handle());
 const std::array<unsigned char, 4> white{255, 255, 255, 255};
 ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white.data());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 client::render::FrameRenderCamera camera;
 camera.farPlane = 2560.0f;
 float projection[16];
 client::render::buildCameraProjection(projection, camera);
 program.bind();
 program.setMatrix4("gbufferProjection", projection);
 program.set1i("lightmap", 0);
 program.set1i("depthtex1", 1);
 program.set1i("fogMode", 0);
 program.set1f("clipDistance", 100.0f);
 core::disableDepthTest();
 core::disableCull();
 core::disableBlend();
 for(const float distance : {50.0f, 1000.0f, 0.0f}) {
  SCOPED_TRACE(distance);
  const float depth = distance == 0.0f ? 1.0f :
      (-projection[10] + projection[14] / distance) * 0.5f + 0.5f;
  std::array<float, 16> samples;
  samples.fill(depth);
  core::activeTexture(0x84C1);
  core::bindTexture(nativeDepth.handle());
  ::glTexImage2D(GL_TEXTURE_2D, 0, 0x8CAC, 4, 4, 0, GL_DEPTH_COMPONENT, GL_FLOAT, samples.data());
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  targets.clearColors({true}, {{{1.0f, 0.0f, 0.0f, 1.0f}}});
  targets.bindGbuffers();
  ::glViewport(0, 0, 4, 4);
  core::drawFullscreen();
  std::array<unsigned char, 4> pixel{};
  ::glReadBuffer(0x8CE0);
  ::glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
  EXPECT_EQ(pixel[0], distance == 50.0f ? 255 : 0);
  EXPECT_EQ(pixel[2], distance == 50.0f ? 0 : 255);
  EXPECT_EQ(::glGetError(), 0u);
 }
 client::gl::ShaderProgram::unbind();
 targets.endGbuffers();
 targets.destroy();
 core::activeTexture(0x84C0);
}
TEST_F(DhProgramTransform, NativeTerrainCrossfadesOnlyWhereDistantCoverageExists) {
 PackInstance pack;
 pack.directory = true;
 pack.definition.distantHorizons = true;
 client::gl::ShaderProgram native;
 client::gl::ShaderProgram distant;
 ASSERT_TRUE(native.compile(surfaceVertex, vanillaFragment("gbuffers_terrain", pack.definition), "#version 430 core\n"))
     << native.lastError();
 ASSERT_TRUE(distant.compile(surfaceVertex, vanillaFragment("dh_terrain", pack.definition), "#version 430 core\n"))
     << distant.lastError();
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(8, 8, {client::render::ColorFormat::Rgba8}, 1));
 client::gl::GlTexture diffuse(core::genTexture());
 client::gl::GlTexture coverage(core::genTexture());
 core::activeTexture(0x84C0);
 core::bindTexture(diffuse.handle());
 const std::array<unsigned char, 4> white{255, 255, 255, 255};
 ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white.data());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 pack.lodCoverageTexture = coverage.handle();
 pack.lodCoverageOrigin[0] = 176.0f;
 pack.lodCoverageOrigin[1] = -8.0f;
 pack.lodTransition[0] = 192.0f;
 pack.lodTransition[1] = 240.0f;
 client::render::WorldProgramBindContext context;
 context.pack = &pack;
 const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
 core::disableDepthTest();
 core::disableCull();
 core::disableBlend();
 for(const bool ready : {false, true}) {
  core::activeTexture(0x84C4);
  core::bindTexture(coverage.handle());
  std::array<unsigned char, 16> cells;
  cells.fill(ready ? 255 : 0);
  ::glTexImage2D(GL_TEXTURE_2D, 0, 0x822B, 8, 1, 0, 0x8227, GL_UNSIGNED_BYTE, cells.data());
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  for(const auto [distance, redPixels] : {std::pair{192.0f, 64}, std::pair{204.0f, 54},
                                         std::pair{216.0f, 32}, std::pair{228.0f, 10}, std::pair{240.0f, 0}}) {
   SCOPED_TRACE(::testing::Message() << ready << ' ' << distance);
   targets.clearColors({true}, {{{0.0f, 1.0f, 0.0f, 1.0f}}});
   targets.bindGbuffers();
   ::glViewport(0, 0, 8, 8);
   for(auto* program : {&distant, &native}) {
    program->bind();
    bindWorldProgram(*program, context);
    program->set1f("testDistance", distance);
    program->set1f("testShade", 1.0f);
    program->set3f("testLight", 1.0f, 1.0f, 1.0f);
    program->set4f("testColor", program == &native ? 1.0f : 0.0f, 0.0f, program == &distant ? 1.0f : 0.0f, 1.0f);
    program->set1i("gtexture", 0);
    program->set1i("fogMode", 0);
    program->set1f("clipDistance", 192.0f);
    program->setMatrix4("gbufferModelViewInverse", identity);
    core::drawFullscreen();
   }
   std::array<unsigned char, 8 * 8 * 4> pixels{};
   ::glReadBuffer(0x8CE0);
   ::glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
   int red = 0;
   for(std::size_t p = 0; p < pixels.size(); p += 4) {
    if(pixels[p] == 255) ++red;
    EXPECT_EQ(pixels[p + 1], 0);
    EXPECT_EQ(pixels[p] + pixels[p + 2], 255);
   }
   EXPECT_EQ(red, ready ? redPixels : 64);
   EXPECT_EQ(::glGetError(), 0u);
  }
 }
 auto shadowCamera = core::cameraFrame();
 shadowCamera.shadowPass = true;
 core::setCameraFrame(shadowCamera);
 native.bind();
 bindWorldProgram(native, context);
 int enabled = 1;
 const auto getUniform = reinterpret_cast<void (*)(unsigned int, int, int*)>(glfwGetProcAddress("glGetUniformiv"));
 ASSERT_NE(getUniform, nullptr);
 getUniform(native.handle(), native.location("betaLodEnabled"), &enabled);
 EXPECT_EQ(enabled, 0);
 core::setCameraFrame({});
 client::gl::ShaderProgram::unbind();
 targets.endGbuffers();
 core::activeTexture(0x84C0);
}

TEST_F(DhProgramTransform, WaterAndIceCrossfadeWithoutApplyingAlphaTwice) {
 PackInstance pack;
 pack.directory = true;
 pack.definition.distantHorizons = true;
 client::gl::ShaderProgram native;
 client::gl::ShaderProgram distant;
 ASSERT_TRUE(native.compile(surfaceVertex, vanillaFragment("gbuffers_water", pack.definition), "#version 430 core\n"))
     << native.lastError();
 ASSERT_TRUE(distant.compile(surfaceVertex, vanillaFragment("dh_water", pack.definition), "#version 430 core\n"))
     << distant.lastError();
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(8, 8, {client::render::ColorFormat::Rgba8}, 1));
 client::gl::GlTexture diffuse(core::genTexture());
 client::gl::GlTexture coverage(core::genTexture());
 client::gl::GlTexture backgroundDepth(core::genTexture());
 core::activeTexture(0x84C4);
 core::bindTexture(coverage.handle());
 std::array<unsigned char, 16> cells;
 cells.fill(255);
 ::glTexImage2D(GL_TEXTURE_2D, 0, 0x822B, 8, 1, 0, 0x8227, GL_UNSIGNED_BYTE, cells.data());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 core::activeTexture(0x84C5);
 core::bindTexture(backgroundDepth.handle());
 std::array<float, 64> depths;
 depths.fill(1.0f);
 ::glTexImage2D(GL_TEXTURE_2D, 0, 0x8CAC, 8, 8, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 pack.lodCoverageTexture = coverage.handle();
 pack.lodCoverageOrigin[0] = 176.0f;
 pack.lodCoverageOrigin[1] = -8.0f;
 pack.lodTransition[0] = 192.0f;
 pack.lodTransition[1] = 240.0f;
 client::render::WorldProgramBindContext context;
 context.pack = &pack;
 context.sceneTargets = &targets;
 context.opaqueDepthTexture = static_cast<int>(backgroundDepth.handle());
 context.dhDepthTextureNoTranslucent = static_cast<int>(backgroundDepth.handle());
 client::render::FrameRenderCamera camera;
 camera.farPlane = 4096.0f;
 float projection[16];
 client::render::buildCameraProjection(projection, camera);
 const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
 core::disableDepthTest();
 core::disableCull();
 core::enableBlend();
 core::blendAlpha();
 for(const int alpha : {146, 160}) {
  core::activeTexture(0x84C0);
  core::bindTexture(diffuse.handle());
  const std::array<unsigned char, 4> blue{0, 0, 255, static_cast<unsigned char>(alpha)};
  ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blue.data());
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  for(const float distance : {192.0f, 204.0f, 216.0f, 228.0f, 240.0f}) {
   SCOPED_TRACE(::testing::Message() << alpha << ' ' << distance);
   targets.clearColors({true}, {{{0.0f, 1.0f, 0.0f, 1.0f}}});
   targets.bindGbuffers();
   ::glViewport(0, 0, 8, 8);
   for(auto* program : {&distant, &native}) {
    program->bind();
    bindWorldProgram(*program, context);
    program->set1f("testDistance", distance);
    program->set1f("testShade", 1.0f);
    program->set3f("testLight", 1.0f, 1.0f, 1.0f);
    program->set4f("testColor", program == &native ? 1.0f : 0.0f, program == &native ? 1.0f : 0.0f,
                   1.0f, program == &native ? 1.0f : static_cast<float>(alpha) / 255.0f);
    program->set1i("gtexture", 0);
    program->set1i("fogMode", 0);
    program->set1f("clipDistance", 192.0f);
    program->setMatrix4("gbufferProjection", projection);
    program->setMatrix4("dhProjection", projection);
    program->setMatrix4("gbufferModelViewInverse", identity);
    core::drawFullscreen();
   }
   std::array<unsigned char, 8 * 8 * 4> pixels{};
   ::glReadBuffer(0x8CE0);
   ::glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
   for(std::size_t p = 0; p < pixels.size(); p += 4) {
    ASSERT_EQ(pixels[p], 0);
    ASSERT_NEAR(pixels[p + 1], 255 - alpha, 1);
    ASSERT_NEAR(pixels[p + 2], alpha, 1);
   }
   EXPECT_EQ(::glGetError(), 0u);
  }
 }
 core::disableBlend();
 client::gl::ShaderProgram::unbind();
 targets.endGbuffers();
 core::activeTexture(0x84C0);
}

TEST_F(DhProgramTransform, DistantShadowsSoftenWhileTopFacesAndLightLevelsStayIntact) {
 PackDefinition definition;
 definition.distantHorizons = true;
 client::gl::ShaderProgram program;
 ASSERT_TRUE(program.compile(surfaceVertex, vanillaFragment("dh_terrain", definition), "#version 430 core\n"))
     << program.lastError();
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 core::disableDepthTest();
 core::disableCull();
 core::disableBlend();
 program.bind();
 program.set1i("betaLodEnabled", 1);
 program.set2f("betaLodTransition", 192.0f, 240.0f);
 program.set1f("clipDistance", 192.0f);
 program.set1i("fogMode", 0);
 for(const float light : {0.1f, 0.5f, 1.0f}) {
  for(const float shade : {0.5f, 0.6f, 0.8f, 1.0f}) {
   for(const bool distantView : {false, true}) {
    SCOPED_TRACE(::testing::Message() << light << ' ' << shade << ' ' << distantView);
    program.set3f("testLight", light, light, light);
    program.set4f("testColor", shade, shade, shade, 1.0f);
    program.set1f("testShade", shade);
    program.set1f("testDistance", distantView ? 496.0f : 240.0f);
    targets.bindGbuffers();
    ::glViewport(0, 0, 4, 4);
    core::drawFullscreen();
    std::array<unsigned char, 4> pixel{};
    ::glReadBuffer(0x8CE0);
    ::glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    const float expected = (distantView ? (1.0f + shade) * 0.5f : shade) * light * 255.0f;
    for(int channel = 0; channel < 3; ++channel) EXPECT_NEAR(pixel[channel], expected, 1.0f);
    EXPECT_EQ(::glGetError(), 0u);
   }
  }
 }
 client::gl::ShaderProgram::unbind();
 targets.endGbuffers();
}

TEST_F(DhProgramTransform, DisabledProviderSkipsRealPackProgramsAndCanBeReenabled) {
 for(const auto& [name, directory] : std::vector<std::pair<std::string, std::string>>{
         {"ComplementaryReimagined_r5.8.1", "world0"},
         {"RenderPearl v2.8.0-beta.4", "world_default"}}) {
  SCOPED_TRACE(name);
  PackInstance pack;
  pack.path = packRoot(name);
  pack.directory = true;
  ASSERT_TRUE(std::filesystem::is_directory(pack.path));
  const auto resources = client::render::PackCatalog::directoryResources(pack.path);
  ASSERT_TRUE(PackLoader::load(resources, [&pack](std::string_view path) {
    return PackCompiler::cachedText(pack, std::string(path));
   }, pack.definition, pack.sourceOptions, pack.summary.error)) << pack.summary.error;
  pack.summary.valid = true;
  for(const std::string program : {"dh_terrain", "dh_water"}) {
   auto& source = pack.definition.programs[program];
   source.vertex = "shaders/" + directory + "/" + program + ".vsh";
   source.fragment = "shaders/" + directory + "/" + program + ".fsh";
  }
  pack.resetPrograms();
  std::string failure;
  const auto log = [&failure](PackInstance&, const std::string& message, ::net::minecraft::util::logging::LogLevel) {
   failure += message + "\n";
  };
  for(const bool enabled : {false, true, false, true}) {
   pack.definition.distantHorizons = enabled;
   pack.resetPrograms();
   for(const std::string program : {"dh_terrain", "dh_water"}) {
    const auto* compiled = PackCompiler::compile(pack, program, log);
    if(enabled) {
     EXPECT_NE(compiled, nullptr) << program << "\n" << failure;
    } else {
     EXPECT_EQ(compiled, nullptr);
    }
   }
   if(!enabled) {
    EXPECT_TRUE(pack.compiledPrograms.empty());
    EXPECT_TRUE(pack.preparedSourceCache.empty());
   }
   EXPECT_TRUE(failure.empty()) << failure;
  }
 }
}
}
