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
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
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
  EXPECT_NE(PackCompiler::compile(pack, "dh_terrain", log), nullptr) << failure;
  EXPECT_NE(PackCompiler::compile(pack, "dh_water", log), nullptr) << failure;
 }
}
}
