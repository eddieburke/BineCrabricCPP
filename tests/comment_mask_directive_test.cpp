// Pins the comment rule for const directives: /* */ hides a directive from the
// GLSL compiler but NOT from the loader (packs rely on that), while // is the
// pack switching the directive off and must still be obeyed.
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
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
// Minimal pack: one composite program carrying the directives under test.
PackDefinition loadWithCompositeSource(const std::string& compositeFsh, std::string& error) {
 const std::vector<std::string> resources = {"shaders/composite.fsh", "shaders/final.fsh"};
 const auto readText = [&compositeFsh](std::string_view path) -> std::string {
  if(path == "shaders/composite.fsh") return compositeFsh;
  return "#version 120\nvoid main(){gl_FragColor=vec4(1.0);}\n";
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 PackLoader::load(resources, readText, pack, options, error);
 return pack;
}
std::string formatOf(const PackDefinition& pack, const std::string& target) {
 const auto found = pack.targets.find(target);
 return found == pack.targets.end() ? std::string{} : found->second.format;
}
// Loading a real pack runs a GPU feature check (COMPUTE_SHADERS, SSBO, ...) and
// bails before scanning any program if it fails, so the end-to-end test needs a
// context. Same hidden-window fixture as ShaderpackLoadPerf.
class RealPackFormats : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "comment-mask-formats", nullptr, nullptr);
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
GLFWwindow* RealPackFormats::window_ = nullptr;
} // namespace
// The RenderPearl shape: the whole directive block lives inside /* */ so the
// GLSL compiler never sees it. Skipping block comments dropped every one of
// these to the RGBA8 default, which is how an HDR bloom buffer ends up 8-bit.
TEST(CommentMaskDirective, BlockCommentedFormatsAreStillRead) {
 const std::string source = R"(#version 120
/*
 const int colortex1Format = RGBA16F;
 const int colortex2Format = RGBA32UI;
*/
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string error;
 const PackDefinition pack = loadWithCompositeSource(source, error);
 EXPECT_EQ(formatOf(pack, "colortex1"), "RGBA16F") << error;
 EXPECT_EQ(formatOf(pack, "colortex2"), "RGBA32UI") << error;
}
// A // directive is the pack turning the setting OFF. Reading through it would
// silently re-enable settings the author disabled on purpose.
TEST(CommentMaskDirective, LineCommentedFormatsAreIgnored) {
 const std::string source = R"(#version 120
// const int colortex1Format = RGBA16F;
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string error;
 const PackDefinition pack = loadWithCompositeSource(source, error);
 EXPECT_NE(formatOf(pack, "colortex1"), "RGBA16F") << error;
}
// The case that makes the two rules collide, and the one RenderPearl actually
// ships: a // disabled directive nested inside the /* */ block. The block must
// be read through, but the nested // must still win for its own line.
TEST(CommentMaskDirective, LineCommentNestedInBlockCommentStaysDisabled) {
 const std::string source = R"(#version 120
/*
 const int colortex1Format = RGBA16F;
 // const int colortex2Format = RGBA32UI;
 const int colortex3Format = RGBA16F;
*/
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string error;
 const PackDefinition pack = loadWithCompositeSource(source, error);
 EXPECT_EQ(formatOf(pack, "colortex1"), "RGBA16F") << error;
 EXPECT_NE(formatOf(pack, "colortex2"), "RGBA32UI") << error;
 // The nested // must not swallow the rest of the block.
 EXPECT_EQ(formatOf(pack, "colortex3"), "RGBA16F") << error;
}
// */ closes the block even mid-way through a nested // run, same as C. If the
// mask kept treating the tail as commented, every directive after it is lost.
TEST(CommentMaskDirective, BlockCloseInsideNestedLineCommentEndsTheBlock) {
 const std::string source = R"(#version 120
/* // disabled */
const int colortex1Format = RGBA16F;
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string error;
 const PackDefinition pack = loadWithCompositeSource(source, error);
 EXPECT_EQ(formatOf(pack, "colortex1"), "RGBA16F") << error;
}
// Clear/ClearColor ride in the same hidden block as the formats. colortexNClear
// = false being dropped means the buffer gets wiped every frame, which reads as
// flicker rather than as a missing directive.
TEST(CommentMaskDirective, BlockCommentedClearDirectivesAreRead) {
 const std::string source = R"(#version 120
/*
 const bool colortex1Clear = false;
 const int colortex1Format = RGBA16F;
*/
void main(){ gl_FragData[0] = vec4(1.0); }
)";
 std::string error;
 const PackDefinition pack = loadWithCompositeSource(source, error);
 const auto found = pack.targets.find("colortex1");
 ASSERT_NE(found, pack.targets.end()) << error;
 EXPECT_FALSE(found->second.clear);
}
// Ground truth: the real pack on disk. rethinking-voxels wraps all 16 of its
// colortexNFormat declarations in one /* */ block, so this is the end-to-end
// check that they survive the whole load. An RGBA8 here means every buffer in
// the pipeline silently degraded: HDR clipped, the TAA buffer quantised, the
// SNORM normal buffer stripped of its sign, and colortex9 not an integer
// texture at all so its imageAtomicMax writes nothing.
TEST_F(RealPackFormats, RethinkingVoxelsResolvesItsBlockHiddenFormats) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 ASSERT_FALSE(resources.empty());
 const auto readText = [&root](std::string_view path) {
  std::ifstream file(root / std::filesystem::path(path), std::ios::binary);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 ASSERT_TRUE(PackLoader::load(resources, readText, pack, options, error)) << error;
 EXPECT_EQ(formatOf(pack, "colortex0"), "R11F_G11F_B10F") << "main colour buffer must stay HDR";
 EXPECT_EQ(formatOf(pack, "colortex2"), "RGB16F") << "TAA accumulation buffer";
 EXPECT_EQ(formatOf(pack, "colortex5"), "RGBA8_SNORM") << "normal buffer needs the sign bit";
 EXPECT_EQ(formatOf(pack, "colortex9"), "R32UI") << "atomics target must be an integer texture";
 EXPECT_EQ(formatOf(pack, "colortex10"), "RGBA16F") << "raw block lighting";
}
// RenderPearl hides the same directives but INDENTS them inside the block, and
// declares colortex1 as RGBA16F / colortex2 as RGBA32UI. If these come back
// RGBA8 the pack is running its HDR and packed-uint buffers as 8-bit unorm.
TEST_F(RealPackFormats, RenderPearlResolvesItsBlockHiddenFormats) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 ASSERT_FALSE(resources.empty());
 const auto readText = [&root](std::string_view path) {
  std::ifstream file(root / std::filesystem::path(path), std::ios::binary);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 PackLoader::load(resources, readText, pack, options, error);
 std::printf("[RENDERPEARL] colortex0=%s colortex1=%s colortex2=%s shadowcolor0=%s\n",
             formatOf(pack, "colortex0").c_str(), formatOf(pack, "colortex1").c_str(),
             formatOf(pack, "colortex2").c_str(), formatOf(pack, "shadowcolor0").c_str());
 std::fflush(stdout);
 EXPECT_EQ(formatOf(pack, "colortex1"), "RGBA16F");
 EXPECT_EQ(formatOf(pack, "colortex2"), "RGBA32UI");
}
} // namespace net::minecraft::test
