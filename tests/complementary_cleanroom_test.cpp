#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/util/logging/Log.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackCompiler;
using client::render::PackInstance;
using client::render::PackLoader;
class ComplementaryCleanroomTest : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "cleanroom-shader-test", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  client::gl::GLCore::ensureLoaded();
  ASSERT_TRUE(client::gl::GLCore::shaderSupported);
 }
 static void TearDownTestSuite() {
  client::render::core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
 }
 void SetUp() override {
  glfwMakeContextCurrent(window_);
  while(::glGetError() != 0u) {}
 }
 static GLFWwindow* window_;
};
GLFWwindow* ComplementaryCleanroomTest::window_ = nullptr;
std::string join(const std::vector<std::string>& lines) {
 std::ostringstream output;
 for(const std::string& line : lines) output << line << '\n';
 return output.str();
}
}
// see third_party/iris/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ProgramDirectives.java:55-76
// Draw buffers come from a RENDERTARGETS/DRAWBUFFERS comment or not at all. A named
// `out_colortexN` layout declaration is not a directive: Java infers nothing from it and
// falls back to draw buffer 0, so a pack that wants more has to say so.
TEST(ComplementaryCleanroomTargets, NamedLayoutOutputsAreNotADrawBufferDirective) {
 const std::string named =
     "layout(location=0) out vec4 out_colortex0;\n"
     "layout(location = 1) out vec4 out_colortex2;\n"
     "layout(location=2) out vec4 out_colortex5;\n";
 EXPECT_TRUE(client::render::parseRenderTargetIndices(named).empty());
 EXPECT_EQ(client::render::renderTargetOutputNames(named), std::vector<std::string>({"colortex0"}));
 const std::string declared = "/* RENDERTARGETS: 0,2,5 */\n" + named;
 EXPECT_EQ(client::render::parseRenderTargetIndices(declared), std::vector<int>({0, 2, 5}));
 EXPECT_EQ(client::render::renderTargetOutputNames(declared),
           std::vector<std::string>({"colortex0", "colortex2", "colortex5"}));
}
TEST_F(ComplementaryCleanroomTest, LoadsHybridDeferredProgramsAndCompilesEveryStage) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "ComplementaryCleanroom";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 PackInstance pack;
 pack.path = root;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 ASSERT_FALSE(resources.empty());
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition,
     pack.sourceOptions,
     pack.summary.error))
     << pack.summary.error;
 EXPECT_TRUE(pack.definition.shadowEnabled);
 EXPECT_EQ(pack.definition.shadowMapResolution, 2048);
 EXPECT_EQ(pack.definition.shadowColorBuffers, 1);
 EXPECT_EQ(pack.definition.gbufferColorBuffers, 6);
 ASSERT_TRUE(pack.definition.targets.contains("colortex0"));
 EXPECT_EQ(pack.definition.targets.at("colortex0").format, "RGBA16F");
 for(int index = 1; index < 6; ++index) {
  const std::string target = "colortex" + std::to_string(index);
  ASSERT_TRUE(pack.definition.targets.contains(target)) << target;
  EXPECT_EQ(pack.definition.targets.at(target).format, "RGBA16F") << target;
 }
 EXPECT_FALSE(pack.definition.targets.at("colortex4").clear);
 EXPECT_FLOAT_EQ(pack.definition.targets.at("colortex5").scaleX, 0.5f);
 EXPECT_FLOAT_EQ(pack.definition.targets.at("colortex5").scaleY, 0.5f);
 EXPECT_TRUE(pack.definition.separateEntityDraws);
 EXPECT_EQ(pack.definition.particleOrdering, "before");
 EXPECT_GE(pack.definition.profiles.size(), 6u);
 const std::vector<std::string> required = {
     "gbuffers_terrain_solid", "gbuffers_terrain_cutout", "gbuffers_water", "gbuffers_gui",
     "gbuffers_gui_textured", "gbuffers_item", "gbuffers_text", "gbuffers_skybasic",
     "gbuffers_skytextured", "gbuffers_basic", "gbuffers_textured", "gbuffers_line",
     "gbuffers_entities", "gbuffers_entities_translucent", "gbuffers_lightning", "gbuffers_block",
     "gbuffers_block_translucent", "gbuffers_particles", "gbuffers_particles_translucent",
     "gbuffers_clouds", "gbuffers_weather", "gbuffers_hand", "gbuffers_damagedblock", "shadow",
     "deferred", "composite", "composite1", "composite2", "final"};
 for(const std::string& name : required) {
  const auto found = pack.definition.programs.find(name);
  ASSERT_NE(found, pack.definition.programs.end()) << name;
  EXPECT_FALSE(found->second.vertex.empty()) << name;
  EXPECT_FALSE(found->second.fragment.empty()) << name;
 }
 const auto passOutput = [&pack](const std::string& name) {
  const auto found = std::find_if(pack.definition.passes.begin(), pack.definition.passes.end(),
                                  [&name](const client::render::PackPass& pass) { return pass.name == name; });
  return found == pack.definition.passes.end() ? std::vector<std::string>{} : found->outputs;
 };
 EXPECT_EQ(passOutput("deferred"), std::vector<std::string>({"colortex0"}));
 EXPECT_EQ(passOutput("composite"), std::vector<std::string>({"colortex3"}));
 EXPECT_EQ(passOutput("composite1"), std::vector<std::string>({"colortex5"}));
 EXPECT_EQ(passOutput("composite2"), std::vector<std::string>({"colortex4"}));
 EXPECT_EQ(passOutput("final"), std::vector<std::string>({"screen"}));
 EXPECT_FALSE(pack.definition.flips.contains("composite2.colortex4"));
 const std::string deferred = PackCompiler::resolveIncludes(pack, "shaders/deferred.fsh");
 const std::string composite = PackCompiler::resolveIncludes(pack, "shaders/composite.fsh");
 const std::string temporal = PackCompiler::resolveIncludes(pack, "shaders/composite2.fsh");
 const std::string final = PackCompiler::resolveIncludes(pack, "shaders/final.fsh");
 for(const char* token : {"colortex1", "colortex2", "depthtex0", "shadowtex0", "shadowcolor0",
                          "CC_COLORED_GI", "CC_SSAO", "CC_PBR"})
  EXPECT_NE(deferred.find(token), std::string::npos) << token;
 for(const char* token : {"CC_SSR", "CC_VOLUMETRIC_SHAFTS", "CC_END_BEAMS", "CC_PUDDLES",
                          "ccTraceReflection", "ccLightShafts", "ccEndBeams"})
  EXPECT_NE(composite.find(token), std::string::npos) << token;
 for(const char* token : {"gbufferPreviousProjection", "gbufferPreviousModelView",
                          "previousCameraPosition", "colortex4"})
  EXPECT_NE(temporal.find(token), std::string::npos) << token;
 for(const char* token : {"CC_MOTION_BLUR", "CC_BLOOM", "CC_FXAA", "ccFilmic"})
  EXPECT_NE(final.find(token), std::string::npos) << token;
 for(const std::string& resource : resources) {
  const std::string source = PackCompiler::readText(pack, resource);
  EXPECT_EQ(source.find("ComplementaryReimagined"), std::string::npos) << resource;
  EXPECT_EQ(source.find("r5.8.1"), std::string::npos) << resource;
 }
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 pack.programs = std::make_unique<client::gl::ProgramCache>(std::filesystem::path{});
 std::vector<std::string> failures;
 std::vector<std::string> names;
 for(const auto& [name, source] : pack.definition.programs) names.push_back(name);
 std::sort(names.begin(), names.end());
 for(const std::string& name : names) {
  const auto log = [&failures, &name](PackInstance&, const std::string& message,
                                      net::minecraft::util::logging::LogLevel) {
   failures.push_back(name + ": " + message);
  };
  if(PackCompiler::compile(pack, name, log) == nullptr) failures.push_back(name + ": returned null");
 }
 EXPECT_TRUE(failures.empty()) << join(failures);
}
}
