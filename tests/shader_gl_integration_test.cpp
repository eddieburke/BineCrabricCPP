#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCatalog.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCompiler.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackLoader.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
using client::render::shaderpack::PackSetting;
using client::render::shaderpack::ShaderPackCatalog::directoryResources;
using client::render::shaderpack::ShaderPackCompiler;
using client::render::shaderpack::ShaderPackDefinition;
using client::render::shaderpack::ShaderPackInstance;
using client::render::shaderpack::ShaderPackLoader;
class ShaderGlIntegrationTest : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "shader-test", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  GLCore::ensureLoaded();
  ASSERT_TRUE(GLCore::shaderSupported);
  ASSERT_TRUE(GLCore::computeSupported);
 }
 static void TearDownTestSuite() {
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
  glfwTerminate();
 }
 static GLFWwindow* window_;
};
GLFWwindow* ShaderGlIntegrationTest::window_ = nullptr;
void mergeDimension(ShaderPackDefinition& target, const ShaderPackDefinition& selected) {
 for(const auto& [name, program] : selected.programs) {
  target.programs[name] = program;
 }
 for(const auto& [name, shaderTarget] : selected.targets) {
  target.targets[name] = shaderTarget;
 }
 target.gbufferColorBuffers = std::max(target.gbufferColorBuffers, selected.gbufferColorBuffers);
 target.shadowColorBuffers = std::max(target.shadowColorBuffers, selected.shadowColorBuffers);
 if(selected.shadowMapResolution > 0) {
  target.shadowMapResolution = selected.shadowMapResolution;
 }
 if(!selected.customUniforms.empty()) {
  target.customUniforms = selected.customUniforms;
 }
 for(const auto& pass : selected.passes) {
  const auto match = std::find_if(target.passes.begin(), target.passes.end(), [&pass](const auto& root) {
   return root.type == pass.type && root.name == pass.name;
  });
  if(match == target.passes.end()) {
   target.passes.push_back(pass);
  } else {
   *match = pass;
  }
 }
}
std::string join(const std::vector<std::string>& lines) {
 std::ostringstream output;
 for(const std::string& line : lines) {
  output << line << '\n';
 }
 return output.str();
}
}
TEST_F(ShaderGlIntegrationTest, DrawBufferBindingQueryIsValid) {
 ASSERT_NE(GLCore::genFramebuffers, nullptr);
 ASSERT_NE(GLCore::bindFramebuffer, nullptr);
 ASSERT_NE(GLCore::deleteFramebuffers, nullptr);
 unsigned int framebuffer = 0;
 GLCore::genFramebuffers(1, &framebuffer);
 ASSERT_NE(framebuffer, 0u);
 GLCore::bindFramebuffer(0x8D40, framebuffer);
 while(::glGetError() != 0) {
 }
 client::gl::ShaderProgram program;
 program.setDrawBufferColortexIndices({0});
 program.applyDrawBuffers(1);
 EXPECT_EQ(::glGetError(), 0u);
 GLCore::bindFramebuffer(0x8D40, 0);
 GLCore::deleteFramebuffers(1, &framebuffer);
}
TEST_F(ShaderGlIntegrationTest, PreamblePublishesEverySupportedExtensionMacro) {
 ShaderPackDefinition definition;
 const std::string source = "#version 430 core\n";
 const std::string preamble =
     client::render::shaderpack::glutil::versionPreamble(definition, source);
 int count = 0;
 ::glGetIntegerv(0x821D, &count);
 ASSERT_GT(count, 0);
 ASSERT_NE(GLCore::getStringi, nullptr);
 std::string first;
 for(int index = 0; index < count; ++index) {
  const unsigned char* bytes = GLCore::getStringi(0x1F03, static_cast<unsigned int>(index));
  ASSERT_NE(bytes, nullptr);
  const std::string extension(reinterpret_cast<const char*>(bytes));
  if(!extension.starts_with("GL_")) continue;
  if(first.empty()) first = extension;
  EXPECT_NE(preamble.find("#define MC_" + extension + "\n"), std::string::npos) << extension;
 }
 ASSERT_FALSE(first.empty());
 const std::string normalized = client::render::shaderpack::glutil::normalizePackSource(
     "#ifdef " + first + "\nraw_extension_enabled\n#endif\n"
     "#ifdef MC_" + first + "\n"
     "iris_extension_enabled\n#endif\n",
     preamble);
 EXPECT_NE(normalized.find("raw_extension_enabled"), std::string::npos);
 EXPECT_NE(normalized.find("iris_extension_enabled"), std::string::npos);
}
TEST_F(ShaderGlIntegrationTest, PreambleMatchesRenderStageAndLabPbrMacros) {
 ShaderPackDefinition definition;
 definition.labPbr13 = true;
 const std::string preamble =
     client::render::shaderpack::glutil::versionPreamble(definition, "#version 430 core\n");
 static constexpr const char* names[] = {
     "NONE",          "SKY",                 "SUNSET",         "CUSTOM_SKY",
     "SUN",           "MOON",                "STARS",          "VOID",
     "TERRAIN_SOLID", "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES",
     "BLOCK_ENTITIES", "DESTROY",             "OUTLINE",        "DEBUG",
     "HAND_SOLID",    "TERRAIN_TRANSLUCENT", "TRIPWIRE",       "PARTICLES",
     "CLOUDS",        "RAIN_SNOW",           "WORLD_BORDER",   "HAND_TRANSLUCENT"};
 for(std::size_t index = 0; index < std::size(names); ++index) {
  EXPECT_NE(preamble.find("#define MC_RENDER_STAGE_" + std::string(names[index]) + " " +
                          std::to_string(index) + "\n"),
            std::string::npos);
 }
 EXPECT_NE(preamble.find("#define MC_TEXTURE_FORMAT_LAB_PBR\n"), std::string::npos);
 EXPECT_NE(preamble.find("#define MC_TEXTURE_FORMAT_LAB_PBR_1_3\n"), std::string::npos);
}
TEST_F(ShaderGlIntegrationTest, CompatibilityGbuffersCompileThroughTheDefaultSourcePath) {
 ShaderPackDefinition definition;
 const std::string vertexSource = R"(#version 430 compatibility
void main() {
 vec3 model = vec3(gl_Vertex);
 vec4 view = gl_ModelViewMatrix * vec4(model, 1.0);
 gl_Position = gl_ProjectionMatrix * view;
 vec2 lm = (gl_TextureMatrix[1] * gl_MultiTexCoord1).xy;
 vec2 uv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
 vec4 col = gl_Color;
 vec3 n = gl_NormalMatrix * gl_Normal;
}
)";
 const std::string fragmentSource = R"(#version 430 compatibility
layout(location = 0) out vec4 color;
void main() {
 color = vec4(1.0);
}
)";
 const std::string preamble =
     client::render::shaderpack::glutil::versionPreamble(definition, vertexSource);
 const std::string vertex = client::render::shaderpack::glutil::prepareSource(
     "gbuffers_terrain", client::render::shaderpack::glutil::ShaderStage::Vertex,
     definition, vertexSource, preamble);
 const std::string fragment = client::render::shaderpack::glutil::prepareSource(
     "gbuffers_terrain", client::render::shaderpack::glutil::ShaderStage::Fragment,
     definition, fragmentSource, preamble);
 client::gl::ShaderProgram program;
 EXPECT_TRUE(program.compile(vertex, fragment, preamble)) << program.lastError();
}
TEST_F(ShaderGlIntegrationTest, RenderPearlCompilesEveryProgramInEveryDimension) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaderpacks" / "RenderPearl v2.8.0-beta.4";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 ShaderPackInstance pack;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_FALSE(resources.empty());
 ASSERT_TRUE(ShaderPackLoader::load(
     resources,
     [&pack](std::string_view path) { return ShaderPackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 std::vector<std::pair<std::string, const ShaderPackDefinition*>> dimensions;
 dimensions.emplace_back("root", nullptr);
 for(const auto& [name, definition] : pack.rootDefinition.dimensionDefinitions) {
  dimensions.emplace_back(name, definition.get());
 }
 std::sort(dimensions.begin(), dimensions.end(), [](const auto& left, const auto& right) {
  return left.first < right.first;
 });
 std::vector<std::string> failures;
 for(const auto& [dimension, selected] : dimensions) {
  pack.compiledPrograms.clear();
  pack.programs = std::make_unique<client::gl::ProgramCache>();
  pack.definition = pack.rootDefinition;
  if(selected != nullptr) {
   mergeDimension(pack.definition, *selected);
  }
  std::vector<std::string> names;
  names.reserve(pack.definition.programs.size());
  for(const auto& [name, ignored] : pack.definition.programs) {
   names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  for(const std::string& name : names) {
   const auto log = [&failures, &dimension](ShaderPackInstance&, const std::string& message) {
    failures.push_back(dimension + ": " + message);
   };
   if(ShaderPackCompiler::compile(pack, name, log) == nullptr &&
      std::none_of(failures.begin(), failures.end(), [&dimension, &name](const std::string& failure) {
       return failure.starts_with(dimension + ": ") && failure.find("program '" + name + "'") != std::string::npos;
      })) {
    failures.push_back(dimension + ": program '" + name + "' returned null");
   }
  }
 }
 EXPECT_TRUE(failures.empty()) << join(failures);
}
}
