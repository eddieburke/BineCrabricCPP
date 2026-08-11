// RenderPearl declares its shadow directives TAB-INDENTED inside a /* */ block.
// Every value asserted here differs from the engine default, so a dropped
// directive fails the assertion rather than coincidentally matching it:
//
//   shadowIntervalSize        default 2.0   pack 0.0
//   shadowHardwareFiltering0  default false pack true
//   shadowHardwareFiltering1  default false pack true
//   shadowDistanceRenderMul   default -1.0  pack 0.85  (this one is at column 0)
//
// shadowIntervalSize is the one that shows: it quantises the shadow map's
// origin to a grid, so losing the pack's 0.0 and falling back to 2.0 makes
// shadows snap in 2-block steps as the player walks.
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackDefinition;
using client::render::PackLoader;
using client::render::PackSourceOption;
PackDefinition loadPack(const std::string& packDirectory, std::string& error) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / packDirectory;
 EXPECT_TRUE(std::filesystem::is_directory(root)) << root.string();
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 const auto readText = [&root](std::string_view path) {
  std::ifstream file(root / std::filesystem::path(path), std::ios::binary);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 // The bool return gates on GPU feature support and needs a context; the const
 // directives are scanned regardless and are what is under test.
 PackLoader::load(resources, readText, pack, options, error);
 return pack;
}
// Same shape as RenderPearl's prelude/directive.glsl: tab-indented const
// declarations inside a block comment.
PackDefinition loadSynthetic(const std::string& composite, std::string& error) {
 const std::vector<std::string> resources = {"shaders/composite.fsh", "shaders/final.fsh"};
 const auto readText = [&composite](std::string_view path) -> std::string {
  if(path == "shaders/composite.fsh") return composite;
  return "#version 120\nvoid main(){gl_FragColor=vec4(1.0);}\n";
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 PackLoader::load(resources, readText, pack, options, error);
 return pack;
}
// RenderPearl uses compute shaders, so PackLoader::load runs a GPU feature check
// and bails BEFORE building the dimension definitions if it fails. Without a
// context every dimension definition is missing and the pack looks broken for
// the wrong reason.
class RenderPearlPack : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "renderpearl-directives", nullptr, nullptr);
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
GLFWwindow* RenderPearlPack::window_ = nullptr;
} // namespace
// The headline: does the real pack's shadow configuration survive the load?
TEST_F(RenderPearlPack, RealPackShadowDirectivesSurvive) {
 std::string error;
 const PackDefinition root = loadPack("RenderPearl v2.8.0-beta.4", error);
 // RenderPearl ships no top-level programs: everything lives under
 // world_default/, so the definition the game actually renders from is the
 // dimension one. That is the unit under test.
 ASSERT_FALSE(root.dimensionDefinitions.empty())
     << "expected a dimension-folder definition for RenderPearl";
 const PackDefinition& pack = *root.dimensionDefinitions.begin()->second;
 EXPECT_FLOAT_EQ(pack.shadowIntervalSize, 0.0f)
     << "fell back to the 2.0 default: the shadow map will snap to a 2-block grid "
        "and shadows will jitter as the player walks";
 EXPECT_TRUE(pack.shadowHardwareFiltering[0]) << "lost shadowHardwareFiltering0 -> no PCF";
 EXPECT_TRUE(pack.shadowHardwareFiltering[1]) << "lost shadowHardwareFiltering1 -> no PCF";
 EXPECT_FLOAT_EQ(pack.shadowDistanceRenderMul, 0.85f);
}
// Isolates indentation as the variable: identical directives, one block
// tab-indented and one at column 0. If indentation were the problem these two
// would disagree.
TEST(RenderPearlShadowDirectives, TabIndentedDirectivesParseSameAsColumnZero) {
 const std::string indented = R"(#version 120
/*
	const float shadowIntervalSize = 0.0;
	const bool shadowHardwareFiltering0 = true;
	const int shadowMapResolution = 4096;
*/
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 const std::string flush = R"(#version 120
/*
const float shadowIntervalSize = 0.0;
const bool shadowHardwareFiltering0 = true;
const int shadowMapResolution = 4096;
*/
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string errorA;
 std::string errorB;
 const PackDefinition tabbed = loadSynthetic(indented, errorA);
 const PackDefinition plain = loadSynthetic(flush, errorB);
 // Every value differs from its default, so "parsed" and "defaulted" are
 // distinguishable.
 EXPECT_FLOAT_EQ(tabbed.shadowIntervalSize, 0.0f) << "tab-indented shadowIntervalSize was dropped";
 EXPECT_TRUE(tabbed.shadowHardwareFiltering[0]) << "tab-indented shadowHardwareFiltering0 was dropped";
 EXPECT_EQ(tabbed.shadowMapResolution, 4096) << "tab-indented shadowMapResolution was dropped";
 EXPECT_FLOAT_EQ(tabbed.shadowIntervalSize, plain.shadowIntervalSize);
 EXPECT_EQ(tabbed.shadowHardwareFiltering[0], plain.shadowHardwareFiltering[0]);
 EXPECT_EQ(tabbed.shadowMapResolution, plain.shadowMapResolution);
}
// Guards the assumption the two tests above rest on: these defaults must stay
// different from RenderPearl's values, or the assertions stop proving anything.
TEST(RenderPearlShadowDirectives, DefaultsDifferFromThePackValuesSoTheTestsCanFail) {
 const PackDefinition fresh;
 EXPECT_FLOAT_EQ(fresh.shadowIntervalSize, 2.0f)
     << "if this default becomes 0.0 the RealPack assertion passes for free — pick another value";
 EXPECT_FALSE(fresh.shadowHardwareFiltering[0]);
 EXPECT_FALSE(fresh.shadowHardwareFiltering[1]);
 EXPECT_FLOAT_EQ(fresh.shadowDistanceRenderMul, -1.0f);
}
} // namespace net::minecraft::test
