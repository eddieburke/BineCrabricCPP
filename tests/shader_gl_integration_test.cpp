#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "support/glsl_snippets_test_fixture.hpp"
namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
using client::render::PackCompiler;
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::PackSetting;
using client::render::PackCatalog::directoryResources;
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
void mergeDimension(PackDefinition& target, const PackDefinition& selected) {
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
} // namespace
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
TEST_F(ShaderGlIntegrationTest, PipelineResetInvalidatesTextureBindings) {
 unsigned int textures[2]{};
 ::glGenTextures(2, textures);
 ASSERT_NE(textures[0], 0u);
 ASSERT_NE(textures[1], 0u);
 client::render::core::activeTexture(0);
 client::render::core::bindTexture(static_cast<int>(textures[0]));
 ::glBindTexture(0x0DE1, textures[1]);
 client::render::Pipeline pipeline(nullptr);
 pipeline.reset();
 client::render::core::activeTexture(0);
 client::render::core::bindTexture(static_cast<int>(textures[0]));
 int bound = 0;
 ::glGetIntegerv(0x8069, &bound);
 EXPECT_EQ(bound, static_cast<int>(textures[0]));
 client::render::core::deleteTexture(textures[0]);
 client::render::core::deleteTexture(textures[1]);
}
TEST_F(ShaderGlIntegrationTest, ProgramUniformSnapshotUploadsOncePerProgramPerGeneration) {
  const std::string vertex = "void main() { gl_Position = vec4(0.0); }\n";
  const std::string fragment = "out vec4 color; void main() { color = vec4(1.0); }\n";
  client::gl::ShaderProgram first;
  client::gl::ShaderProgram second;
  ASSERT_TRUE(first.compile(vertex, fragment, "#version 330 core\n"));
  ASSERT_TRUE(second.compile(vertex, fragment, "#version 330 core\n"));
  int snapshotUploads = 0;
  int materialBinds = 0;
  client::render::core::setProgramUniformUploader([&snapshotUploads](client::gl::ShaderProgram&) {
   ++snapshotUploads;
  });
  client::render::core::setProgramMaterialBinder([&materialBinds](client::gl::ShaderProgram&) {
   ++materialBinds;
  });
  client::render::core::RenderPass pass;
  client::render::core::setActiveProgram(&first);
  client::render::core::bindAndUploadUniforms(pass);
  client::render::core::setActiveProgram(&second);
  client::render::core::bindAndUploadUniforms(pass);
  client::render::core::setActiveProgram(&first);
  client::render::core::bindAndUploadUniforms(pass);
  EXPECT_EQ(snapshotUploads, 2) << "bouncing A->B->A must not re-upload A's snapshot";
  EXPECT_EQ(materialBinds, 1) << "first draw initializes the diffuse-texture cache";
  client::render::core::advanceProgramUniforms();
  client::render::core::bindAndUploadUniforms(pass);
  EXPECT_EQ(snapshotUploads, 3) << "generation advance re-uploads the active program";
  EXPECT_EQ(materialBinds, 2) << "generation advance also refreshes the diffuse cache";
  unsigned int textures[2]{};
  ::glGenTextures(2, textures);
  client::render::core::activeTexture(0);
  client::render::core::bindTexture(static_cast<int>(textures[0]));
  client::render::core::bindAndUploadUniforms(pass);
  EXPECT_EQ(materialBinds, 3) << "diffuse texture change binds material only";
  EXPECT_EQ(snapshotUploads, 3) << "texture change must not re-upload the snapshot";
  client::render::core::bindAndUploadUniforms(pass);
  EXPECT_EQ(materialBinds, 3) << "same diffuse texture does not re-bind material";
  client::render::core::bindTexture(static_cast<int>(textures[1]));
  client::render::core::bindAndUploadUniforms(pass);
  EXPECT_EQ(materialBinds, 4);
  EXPECT_EQ(snapshotUploads, 3);
  client::render::core::deleteTexture(textures[0]);
  client::render::core::deleteTexture(textures[1]);
  client::render::core::setProgramUniformUploader(nullptr);
  client::render::core::setProgramMaterialBinder(nullptr);
  client::render::core::setActiveProgram(nullptr);
}
TEST_F(ShaderGlIntegrationTest, TextureDeletionClearsEveryTrackedUnit) {
  const unsigned int texture = client::render::core::genTexture();
 ASSERT_NE(texture, 0u);
 client::render::core::activeTexture(0);
 client::render::core::bindTexture(static_cast<int>(texture));
 client::render::core::activeTexture(3);
 client::render::core::bindTexture(static_cast<int>(texture));
 client::render::core::deleteTexture(texture);
 client::render::core::activeTexture(0);
 EXPECT_EQ(client::render::core::boundTexture(), 0);
 client::render::core::activeTexture(3);
 EXPECT_EQ(client::render::core::boundTexture(), 0);
 client::render::core::activeTexture(0);
}
TEST_F(ShaderGlIntegrationTest, LowLevelCompilerRejectsNonCoreDialects) {
 client::gl::ShaderProgram program;
 const std::string vertex = "void main() { gl_Position = vec4(0.0); }\n";
 const std::string fragment = "out vec4 color; void main() { color = vec4(1.0); }\n";
 EXPECT_FALSE(program.compile(vertex, fragment, "#version 120\n"));
 EXPECT_EQ(program.lastError(), "raster shaders require #version 330 core or newer");
 EXPECT_FALSE(program.compile(vertex, fragment, "#version 330 compatibility\n"));
 EXPECT_EQ(program.lastError(), "raster shaders require #version 330 core or newer");
 EXPECT_FALSE(program.compileCompute("void main() {}\n", "#version 420 core\n"));
 EXPECT_EQ(program.lastError(), "compute shaders require #version 430 core or newer");
}
TEST_F(ShaderGlIntegrationTest, CacheCompilesComputePerKey) {
 client::gl::ShaderCompileService compiler;
 client::gl::ProgramCache cache(compiler);
 const std::string source = R"(layout(local_size_x = 1) in;
void main() {}
)";
 const std::string preamble = "#version 430 core\n";
 ASSERT_NE(cache.getFromComputeSource("first", source, preamble), nullptr)
     << cache.compileError("first");
 ASSERT_NE(cache.getFromComputeSource("second", source, preamble), nullptr)
     << cache.compileError("second");
}
TEST_F(ShaderGlIntegrationTest, PrepareWriteCopiesReadTextureToWriteTexture) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}));
 targets.bindGbuffers();
 const float red[4] = {0.75f, 0.25f, 0.125f, 1.0f};
 GLCore::clearBufferfv(0x1800, 0, red);
 targets.endGbuffers();
 while(::glGetError() != 0) {
 }
 targets.prepareWrite("colortex0");
 EXPECT_EQ(::glGetError(), 0u);
 client::render::core::activeTexture(0);
 client::render::core::bindTexture(static_cast<int>(targets.writeTexture(0)));
 std::array<unsigned char, 4 * 4 * 4> pixels{};
 ::glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, pixels.data());
 EXPECT_NEAR(pixels[0], 191, 1);
 EXPECT_NEAR(pixels[1], 64, 1);
 EXPECT_NEAR(pixels[2], 32, 1);
 EXPECT_EQ(pixels[3], 255);
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, RenderPearlRgba16fPrepareWriteAndImageBinding) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8, client::render::ColorFormat::Rgba16F}));
 targets.bindGbuffers();
 const float source[4] = {0.25f, 0.5f, 0.75f, 1.0f};
 GLCore::clearBufferfv(0x1800, 1, source);
 targets.endGbuffers();
 while(::glGetError() != 0) {
 }
 targets.prepareWrite("colortex1");
 ASSERT_EQ(::glGetError(), 0u);
 client::gl::ShaderProgram program;
 ASSERT_TRUE(program.compileCompute(
     R"(layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba16f) uniform image2D colorimg1;
void main() {
 imageStore(colorimg1, ivec2(gl_GlobalInvocationID.xy), vec4(0.125, 0.25, 0.5, 1.0));
})",
     "#version 430 core\n"))
     << program.lastError();
 program.bind();
 std::unordered_map<std::string, int> images{{"colortex1", static_cast<int>(targets.writeTexture(1))}};
 ASSERT_EQ(client::render::bindColorImages(program, images, PackDefinition{}, &targets), 1u);
 ASSERT_EQ(::glGetError(), 0u);
 GLCore::dispatchCompute(4, 4, 1);
 GLCore::memoryBarrier(0xFFFFFFFFu);
 ASSERT_EQ(::glGetError(), 0u);
 client::render::core::activeTexture(0);
 client::render::core::bindTexture(static_cast<int>(targets.writeTexture(1)));
 std::array<float, 4 * 4 * 4> pixels{};
 ::glGetTexImage(0x0DE1, 0, 0x1908, 0x1406, pixels.data());
 EXPECT_NEAR(pixels[0], 0.125f, 0.001f);
 EXPECT_NEAR(pixels[1], 0.25f, 0.001f);
 EXPECT_NEAR(pixels[2], 0.5f, 0.001f);
 EXPECT_NEAR(pixels[3], 1.0f, 0.001f);
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, PreamblePublishesEverySupportedExtensionMacro) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition definition;
 const std::string source = "#version 430 core\n";
 const std::string preamble =
     client::render::versionPreamble(definition, source);
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
 const std::string normalized = client::render::normalizePackSource(
     definition,
     "#ifdef " + first + "\nraw_extension_enabled\n#endif\n"
                         "#ifdef MC_" +
         first + "\n"
                 "iris_extension_enabled\n#endif\n");
 EXPECT_NE(normalized.find("raw_extension_enabled"), std::string::npos);
 EXPECT_NE(normalized.find("iris_extension_enabled"), std::string::npos);
}
TEST_F(ShaderGlIntegrationTest, PreambleMatchesRenderStageAndLabPbrMacros) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition definition;
 definition.labPbr13 = true;
 const std::string preamble =
     client::render::versionPreamble(definition, "#version 430 core\n");
 static constexpr const char* names[] = {
     "NONE", "SKY", "SUNSET", "CUSTOM_SKY",
     "SUN", "MOON", "STARS", "VOID",
     "TERRAIN_SOLID", "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES",
     "BLOCK_ENTITIES", "DESTROY", "OUTLINE", "DEBUG",
     "HAND_SOLID", "TERRAIN_TRANSLUCENT", "TRIPWIRE", "PARTICLES",
     "CLOUDS", "RAIN_SNOW", "WORLD_BORDER", "HAND_TRANSLUCENT"};
 for(std::size_t index = 0; index < std::size(names); ++index) {
  EXPECT_NE(preamble.find("#define MC_RENDER_STAGE_" + std::string(names[index]) + " " +
                          std::to_string(index) + "\n"),
            std::string::npos);
 }
 EXPECT_NE(preamble.find("#define MC_TEXTURE_FORMAT_LAB_PBR\n"), std::string::npos);
 EXPECT_NE(preamble.find("#define MC_TEXTURE_FORMAT_LAB_PBR_1_3\n"), std::string::npos);
}
TEST_F(ShaderGlIntegrationTest, CompatibilityGbuffersCompileThroughTheDefaultSourcePath) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition definition;
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
     client::render::versionPreamble(definition, vertexSource);
 const std::string vertex = client::render::prepareSource(
     "gbuffers_terrain", client::render::ShaderStage::Vertex,
     definition, vertexSource);
 const std::string fragment = client::render::prepareSource(
     "gbuffers_terrain", client::render::ShaderStage::Fragment,
     definition, fragmentSource);
 client::gl::ShaderProgram program;
 EXPECT_TRUE(program.compile(vertex, fragment, preamble)) << program.lastError();
}
TEST_F(ShaderGlIntegrationTest, RenderPearlCompilesEveryProgramInEveryDimension) {
 net::minecraft::test::installTestGlslSnippets();
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 PackInstance pack;
 client::gl::ShaderCompileService compiler;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_FALSE(resources.empty());
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 std::vector<std::pair<std::string, const PackDefinition*>> dimensions;
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
  pack.programs = std::make_unique<client::gl::ProgramCache>(compiler);
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
    const auto log = [&failures, &dimension](PackInstance&, const std::string& message,
                                             net::minecraft::util::logging::LogLevel) {
     failures.push_back(dimension + ": " + message);
    };
   if(PackCompiler::compile(pack, name, log) == nullptr &&
      std::none_of(failures.begin(), failures.end(), [&dimension, &name](const std::string& failure) {
       return failure.starts_with(dimension + ": ") && failure.find("program '" + name + "'") != std::string::npos;
      })) {
    failures.push_back(dimension + ": program '" + name + "' returned null");
   }
  }
 }
 EXPECT_TRUE(failures.empty()) << join(failures);
}
TEST_F(ShaderGlIntegrationTest, RenderPearlDeferredExposesImageUniforms) {
 net::minecraft::test::installTestGlslSnippets();
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 PackInstance pack;
 client::gl::ShaderCompileService compiler;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 EXPECT_TRUE(pack.rootDefinition.requiredFeatures.contains("SEPARATE_HARDWARE_SAMPLERS"));
 EXPECT_EQ(pack.rootDefinition.images.size(), 2u);
 EXPECT_TRUE(std::any_of(pack.rootDefinition.customTextures.begin(), pack.rootDefinition.customTextures.end(),
                         [](const auto& texture) { return texture.name == "areatex"; }));
 EXPECT_TRUE(std::any_of(pack.rootDefinition.customTextures.begin(), pack.rootDefinition.customTextures.end(),
                         [](const auto& texture) { return texture.name == "searchtex"; }));
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 pack.programs = std::make_unique<client::gl::ProgramCache>(compiler);
 const auto log = [](PackInstance&, const std::string&, net::minecraft::util::logging::LogLevel) {};
 client::gl::ShaderProgram* deferred = PackCompiler::compile(pack, "deferred#compute", log);
 ASSERT_NE(deferred, nullptr) << "deferred#compute failed to compile";
 ASSERT_TRUE(deferred->valid());
 EXPECT_GE(deferred->location("colorimg1"), 0) << "deferred colorimg1 not found";
 EXPECT_GE(deferred->location("colorimg0"), -1);
 client::gl::ShaderProgram* composite = PackCompiler::compile(pack, "composite#compute", log);
 ASSERT_NE(composite, nullptr) << "composite#compute failed to compile";
 EXPECT_GE(composite->location("colorimg1"), 0) << "composite colorimg1 not found";
}
TEST_F(ShaderGlIntegrationTest, RenderPearlGbufferDrawBuffersParse) {
 net::minecraft::test::installTestGlslSnippets();
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 PackInstance pack;
 client::gl::ShaderCompileService compiler;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 const auto dimension = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(dimension, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *dimension->second);
 ASSERT_TRUE(pack.definition.targets.contains("colortex1"));
 ASSERT_TRUE(pack.definition.targets.contains("colortex2"));
 EXPECT_EQ(pack.definition.targets.at("colortex1").format, "RGBA16F");
 EXPECT_EQ(pack.definition.targets.at("colortex2").format, "RGBA32UI");
 pack.programs = std::make_unique<client::gl::ProgramCache>(compiler);
 const auto log = [](PackInstance&, const std::string&, net::minecraft::util::logging::LogLevel) {};
 client::gl::ShaderProgram* terrain = PackCompiler::compile(pack, "gbuffers_terrain_solid", log);
 ASSERT_NE(terrain, nullptr) << "gbuffers_terrain_solid failed to compile";
 EXPECT_EQ(terrain->drawBufferColortexIndices(), (std::vector<int>{1, 2}));
}
TEST_F(ShaderGlIntegrationTest, RenderPearlDeferredWriteBuffersMatchComposite) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 PackInstance pack;
 client::gl::ShaderCompileService compiler;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 const auto& selected = *found->second;
 PackDefinition merged = pack.rootDefinition;
 for(const auto& [name, program] : selected.programs) merged.programs[name] = program;
 pack.definition = merged;
 pack.programs = std::make_unique<client::gl::ProgramCache>(compiler);
 const auto log = [](PackInstance&, const std::string&, net::minecraft::util::logging::LogLevel) {};
 const auto terrainIt = merged.programs.find("gbuffers_terrain_solid");
 ASSERT_NE(terrainIt, merged.programs.end());
 const std::string src = terrainIt->second.fragment;
 const std::string frag = PackCompiler::resolveIncludes(pack, src);
 const std::string preamble = client::render::versionPreamble(pack.definition, frag);
 const std::string prepared =
     client::render::prepareSource("gbuffers_terrain_solid", client::render::ShaderStage::Fragment,
                                   pack.definition, frag);
 const std::vector<int> targets = client::render::parseRenderTargetIndices(prepared);
 EXPECT_EQ(targets, (std::vector<int>{1, 2}));
 client::gl::ShaderProgram* deferred = PackCompiler::compile(pack, "deferred#compute", log);
 ASSERT_NE(deferred, nullptr);
 int colorCount = 3;
 std::vector<std::string> writeBuffers;
 for(int i = 0; i < colorCount; ++i) {
  if(deferred->location("colorimg" + std::to_string(i)) < 0) continue;
  writeBuffers.push_back("colortex" + std::to_string(i));
 }
 EXPECT_EQ(writeBuffers, (std::vector<std::string>{"colortex1"}));
 EXPECT_EQ(pack.definition.bufferObjects.size(), 5u);
 EXPECT_EQ(pack.definition.images.size(), 2u);
}
TEST_F(ShaderGlIntegrationTest, RenderPearlListsComputePassesAndPrograms) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 PackInstance pack;
 pack.path = root;
 pack.directory = true;
 const auto resources = directoryResources(root);
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 for(const auto& [name, program] : pack.definition.programs) {
  if(name.find("#compute") == std::string::npos) continue;
  const std::string src = program.compute;
  // Print which pass references it
  for(const auto& pass : pack.definition.passes) {
   if(pass.program == name) {
    printf("[diag] pass=%s program=%s source=%s\n", pass.name.c_str(), name.c_str(), src.c_str());
   }
  }
 }
}
} // namespace net::minecraft::test
