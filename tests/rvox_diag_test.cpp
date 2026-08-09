#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
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
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackCompiler;
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::PackSetting;
using client::render::PackCatalog::zipResources;
class RvoxDiag : public ::testing::Test {
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
GLFWwindow* RvoxDiag::window_ = nullptr;
void mergeDimension(PackDefinition& target, const PackDefinition& selected) {
 for(const auto& [name, program] : selected.programs) target.programs[name] = program;
 for(const auto& [name, shaderTarget] : selected.targets) target.targets[name] = shaderTarget;
 target.gbufferColorBuffers = std::max(target.gbufferColorBuffers, selected.gbufferColorBuffers);
 target.shadowColorBuffers = std::max(target.shadowColorBuffers, selected.shadowColorBuffers);
 if(selected.shadowMapResolution > 0) target.shadowMapResolution = selected.shadowMapResolution;
 if(!selected.customUniforms.empty()) target.customUniforms = selected.customUniforms;
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
void dumpProgram(PackInstance& pack, const std::string& name, const std::filesystem::path& outDir) { const auto found = pack.definition.programs.find(name);
 if(found == pack.definition.programs.end()) {
  printf("[diag] program not found: %s\n", name.c_str());
  return;
 }
 const auto& spec = found->second;
 const std::string vertexIncluded = PackCompiler::resolveIncludes(pack, spec.vertex);
 const std::string fragmentIncluded = PackCompiler::resolveIncludes(pack, spec.fragment);
 const std::string vertexPrepared =
     client::render::prepareSource(name, client::render::ShaderStage::Vertex, pack.definition, vertexIncluded);
 const std::string fragmentPrepared =
     client::render::prepareSource(name, client::render::ShaderStage::Fragment, pack.definition, fragmentIncluded);
 std::ofstream( outDir / (name + ".vsh.expanded")) << vertexIncluded;
 std::ofstream( outDir / (name + ".vsh.prepared")) << vertexPrepared;
 std::ofstream( outDir / (name + ".fsh.expanded")) << fragmentIncluded;
 std::ofstream( outDir / (name + ".fsh.prepared")) << fragmentPrepared;
  printf("[diag] dumped %s (vertex %zu -> %zu, fragment %zu -> %zu)\n", name.c_str(), vertexIncluded.size(),
         vertexPrepared.size(), fragmentIncluded.size(), fragmentPrepared.size());
}
std::string join(const std::vector<std::string>& lines) {
 std::ostringstream output;
 for(const std::string& line : lines) output << line << '\n';
 return output.str();
}
std::filesystem::path shaderTestPack(std::string_view name) {
 if(const char* root = std::getenv("MINECRAFT_SHADER_TEST_ROOT"); root != nullptr && *root != '\0') {
  return std::filesystem::path(root) / name;
 }
 return std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / name;
}
client::gl::GlTexture solidTexture(const std::array<unsigned char, 4>& pixel) {
 const unsigned int handle = client::render::core::genTexture();
 client::render::core::bindTexture(client::gl::cap::Texture2D, static_cast<int>(handle));
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MinFilter, client::gl::filter::Nearest);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MagFilter, client::gl::filter::Nearest);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::WrapS, client::gl::wrap::ClampToEdge);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::WrapT, client::gl::wrap::ClampToEdge);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MaxLevel, 0);
 ::glTexImage2D(client::gl::cap::Texture2D, 0, client::gl::pixel::Rgba8, 1, 1, 0,
                client::gl::pixel::Rgba, client::gl::pixel::UnsignedByte, pixel.data());
 return client::gl::GlTexture(handle);
}
client::gl::GlTexture solidDepthTexture(float depth) {
 const unsigned int handle = client::render::core::genTexture();
 client::render::core::bindTexture(client::gl::cap::Texture2D, static_cast<int>(handle));
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MinFilter, client::gl::filter::Nearest);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MagFilter, client::gl::filter::Nearest);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::WrapS, client::gl::wrap::ClampToEdge);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::WrapT, client::gl::wrap::ClampToEdge);
 ::glTexParameteri(client::gl::cap::Texture2D, client::gl::tex::MaxLevel, 0);
 ::glTexImage2D(client::gl::cap::Texture2D, 0, client::gl::pixel::DepthComponent24, 1, 1, 0,
                client::gl::pixel::DepthComponent, client::gl::pixel::Float, &depth);
 return client::gl::GlTexture(handle);
}
} // namespace
TEST_F(RvoxDiag, DumpPreparedSources) {
 const std::filesystem::path zipPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9.zip";
 const std::filesystem::path dirPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9";
 PackInstance pack;
 if(std::filesystem::exists(zipPath)) {
  pack.path = zipPath;
  pack.directory = false;
  pack.zip = std::make_unique<client::resource::pack::ZippedTexturePack>(zipPath);
  pack.zip->open();
  const std::vector<std::string> resources = zipResources(*pack.zip);
  ASSERT_TRUE(PackLoader::load(
      resources,
      [&pack](std::string_view path) { return PackCompiler::cachedText(pack, std::string(path)); },
      pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
 } else {
  ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << dirPath.string();
  pack.path = dirPath;
  pack.directory = true;
  const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dirPath);
  ASSERT_TRUE(PackLoader::load(
      resources,
      [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
      pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
 }
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *found->second);
 const std::filesystem::path outDir = std::filesystem::temp_directory_path() / "rvox_prepared";
 std::filesystem::create_directories(outDir);
 for(const char* name : {"gbuffers_terrain_solid", "gbuffers_entities", "gbuffers_hand", "shadow", "final",
                         "prepare1", "deferred1", "composite", "gbuffers_weather"}) {
  dumpProgram(pack, name, outDir);
 }
 printf("[diag] output dir: %s\n", outDir.string().c_str());
}
TEST_F(RvoxDiag, CompileAllPrograms) {
  const std::filesystem::path zipPath =
      std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9.zip";
  const std::filesystem::path dirPath =
      shaderTestPack("rethinking-voxels_r0.1-beta9");
  PackInstance pack;
  if(std::filesystem::exists(zipPath)) {
   pack.path = zipPath;
   pack.directory = false;
   pack.zip = std::make_unique<client::resource::pack::ZippedTexturePack>(zipPath);
   pack.zip->open();
   const std::vector<std::string> resources = zipResources(*pack.zip);
   ASSERT_TRUE(PackLoader::load(
       resources,
       [&pack](std::string_view path) { return PackCompiler::cachedText(pack, std::string(path)); },
       pack.definition, pack.sourceOptions, pack.summary.error))
       << pack.summary.error;
  } else {
   ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << dirPath.string();
   pack.path = dirPath;
   pack.directory = true;
   const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dirPath);
   ASSERT_TRUE(PackLoader::load(
       resources,
       [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
       pack.definition, pack.sourceOptions, pack.summary.error))
       << pack.summary.error;
  }
  pack.summary.valid = true;
  pack.rootDefinition = pack.definition;
  for(const PackSetting& setting : pack.rootDefinition.settings) {
   pack.settings.emplace(setting.key, setting.defaultValue);
  }
  const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
  ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
  mergeDimension(pack.definition, *found->second);
  EXPECT_EQ(std::count_if(pack.definition.bufferBlends.begin(), pack.definition.bufferBlends.end(),
                          [](const auto& blend) { return blend.program == "gbuffers_water"; }),
            0);
  std::string runtimeError;
  ASSERT_TRUE(pack.rebuildRuntime(runtimeError)) << runtimeError;
  std::vector<std::string> failures;
  for(const auto& [name, ignored] : pack.definition.programs) {
   (void)ignored;
   std::cout << "[PROGRAM KEY]: " << name << std::endl;
   const auto log = [&failures, &name](PackInstance&, const std::string& message,
                                       net::minecraft::util::logging::LogLevel) {
    failures.push_back(name + ": " + message);
   };
   if(PackCompiler::compile(pack, name, log) == nullptr) {
    failures.push_back(name + ": returned null");
   }
  }
  printf("[diag] compiled %zu programs, %zu failures\n", pack.definition.programs.size(), failures.size());
  for(const std::string& failure : failures) {
   printf("[diag] %s\n", failure.c_str());
  }
  EXPECT_TRUE(failures.empty()) << join(failures);
  ASSERT_TRUE(pack.buildExecutionPlan(runtimeError)) << runtimeError;
  EXPECT_FALSE(pack.stagePlan(client::render::CompositeStage::Prepare).empty());
  EXPECT_FALSE(pack.stagePlan(client::render::CompositeStage::Composite).empty());
  for(const client::render::WorldProgramId id : {client::render::WorldProgramId::TerrainSolid,
                                                 client::render::WorldProgramId::TerrainCutout,
                                                 client::render::WorldProgramId::TerrainTranslucent,
                                                 client::render::WorldProgramId::Entities,
                                                 client::render::WorldProgramId::EntitiesTranslucent,
                                                 client::render::WorldProgramId::Hand}) {
   EXPECT_NE(pack.worldPrograms[static_cast<std::size_t>(id)][0].program, nullptr)
       << client::render::worldProgramKey(id);
  }
}
TEST_F(RvoxDiag, DumpFailingProgramSources) {
 const std::filesystem::path dirPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9";
  ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << dirPath.string();
  PackInstance pack;
  pack.path = dirPath;
  pack.directory = true;
  const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dirPath);
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
  const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
  ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
  mergeDimension(pack.definition, *found->second);
  const std::filesystem::path outDir = std::filesystem::temp_directory_path() / "rvox_failing";
  std::filesystem::create_directories(outDir);
  const char* vendor = reinterpret_cast<const char*>(::glGetString(0x1F00));
  const char* renderer = reinterpret_cast<const char*>(::glGetString(0x1F01));
  const char* version = reinterpret_cast<const char*>(::glGetString(0x1F02));
  printf("[diag] GL_VENDOR=%s GL_RENDERER=%s GL_VERSION=%s\n", vendor ? vendor : "?", renderer ? renderer : "?",
         version ? version : "?");
  int failed = 0;
  for(const auto& [name, spec] : pack.definition.programs) {
   std::string preamble;
   std::string compute;
   std::string vertex;
   std::string fragment;
   std::string geometry;
   std::string tessControl;
   std::string tessEvaluation;
   if(!spec.compute.empty()) {
    const std::string included = PackCompiler::resolveIncludes(pack, spec.compute);
    if(included.empty()) continue;
    preamble = client::render::versionPreamble(pack.definition, included, true);
    compute = client::render::prepareSource(name, client::render::ShaderStage::Compute, pack.definition, included);
   } else {
    if(PackCompiler::resolveIncludes(pack, spec.fragment).empty()) continue;
    const std::string vertexIncluded =
        spec.vertex.empty() ? client::render::defaultRasterVertexShader()
                            : PackCompiler::resolveIncludes(pack, spec.vertex);
    const std::string fragmentIncluded = PackCompiler::resolveIncludes(pack, spec.fragment);
    const std::string geometryIncluded =
        spec.geometry.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.geometry);
    const std::string tessControlIncluded =
        spec.tessControl.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.tessControl);
    const std::string tessEvaluationIncluded =
        spec.tessEvaluation.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.tessEvaluation);
    const bool hasTessellation = !tessControlIncluded.empty() || !tessEvaluationIncluded.empty();
    preamble = client::render::versionPreambleForStages(
        pack.definition,
        {vertexIncluded, fragmentIncluded, geometryIncluded, tessControlIncluded, tessEvaluationIncluded},
        hasTessellation ? 400 : 330);
    const client::render::ShaderTransformContext context = {};
    vertex = client::render::prepareSource(name, client::render::ShaderStage::Vertex, pack.definition,
                                           vertexIncluded, context);
    fragment = client::render::prepareSource(name, client::render::ShaderStage::Fragment, pack.definition,
                                             fragmentIncluded, context);
    geometry = geometryIncluded.empty() ? std::string{}
                                        : client::render::prepareSource(name, client::render::ShaderStage::Geometry,
                                                                        pack.definition, geometryIncluded, context);
    tessControl = tessControlIncluded.empty()
                      ? std::string{}
                      : client::render::prepareSource(name, client::render::ShaderStage::TessControl,
                                                     pack.definition, tessControlIncluded, context);
    tessEvaluation = tessEvaluationIncluded.empty()
                         ? std::string{}
                         : client::render::prepareSource(name, client::render::ShaderStage::TessEvaluation,
                                                        pack.definition, tessEvaluationIncluded, context);
   }
   client::gl::ShaderProgram program;
   const bool ok = !spec.compute.empty()
                       ? program.compileCompute(compute, preamble)
                       : program.compile(vertex, fragment, preamble, geometry, tessControl, tessEvaluation);
   if(ok) continue;
   ++failed;
   std::string stem = name;
   std::replace(stem.begin(), stem.end(), '#', '_');
   const std::string body = !spec.compute.empty() ? compute : vertex;
   std::ofstream(outDir / (stem + ".assembled")) << preamble << body;
   if(!spec.compute.empty()) {
    std::ofstream(outDir / (stem + ".csh")) << compute;
   } else {
    std::ofstream(outDir / (stem + ".vsh")) << vertex;
    std::ofstream(outDir / (stem + ".fsh")) << fragment;
    if(!geometry.empty()) std::ofstream(outDir / (stem + ".gsh")) << geometry;
   }
   std::ofstream(outDir / (stem + ".error")) << program.lastError();
   printf("[diag] %s failed: %s\n", name.c_str(), program.lastError().c_str());
  }
  printf("[diag] failed %d\n", failed);
  printf("[diag] output dir: %s\n", outDir.string().c_str());
}
TEST_F(RvoxDiag, BuiltinBoundsRewrite) {
 PackDefinition definition;
 const std::string in = "float f = clamp(dir.y + 1.6, 0.6, 1);\n"
                        "float g = clamp(a, 0.0, 1);\n"
                        "float h = clamp(b, 0, 1.0);\n"
                        "int i = min(j, 5);\n"
                        "int k = clamp(c, 0, 1);\n";
 const std::string out =
     client::render::prepareSource("gbuffers_terrain", client::render::ShaderStage::Fragment, definition, in);
 printf("[diag] out=%s\n", out.c_str());
 printf("[diag] rewrite3=%d rewrite2=%d rewrite1=%d\n",
        out.find("clamp(dir.y + 1.6, 0.6, 1.0)") != std::string::npos,
        out.find("clamp(a, 0.0, 1.0)") != std::string::npos,
        out.find("clamp(b, 0.0, 1.0)") != std::string::npos);
}
TEST_F(RvoxDiag, EnsureSceneTargetsRthinkingVoxels) {
  while(::glGetError() != 0) {}
  const std::filesystem::path dirPath =
      shaderTestPack("rethinking-voxels_r0.1-beta9");
  ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << dirPath.string();
  PackInstance pack;
  pack.path = dirPath;
  pack.directory = true;
  const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dirPath);
  ASSERT_TRUE(PackLoader::load(
      resources,
      [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
      pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
  pack.summary.valid = true;
  pack.rootDefinition = pack.definition;
  const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
  ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
  mergeDimension(pack.definition, *found->second);
  const auto computePass = [&pack](std::string_view name) {
   return std::find_if(pack.definition.passes.begin(), pack.definition.passes.end(),
                       [name](const auto& pass) { return pass.name == name && pass.type == "compute"; });
  };
  const auto shadowcomp = computePass("shadowcomp");
  ASSERT_NE(shadowcomp, pack.definition.passes.end());
  EXPECT_EQ(shadowcomp->localSize[0], 8);
  EXPECT_EQ(shadowcomp->localSize[1], 8);
  EXPECT_EQ(shadowcomp->localSize[2], 8);
  EXPECT_EQ(shadowcomp->groups[0], 16);
  EXPECT_EQ(shadowcomp->groups[1], 12);
  EXPECT_EQ(shadowcomp->groups[2], 16);
  EXPECT_FALSE(shadowcomp->relativeGroups);
  const auto deferred = computePass("deferred1");
  ASSERT_NE(deferred, pack.definition.passes.end());
  EXPECT_EQ(deferred->localSize[0], 16);
  EXPECT_EQ(deferred->localSize[1], 16);
  EXPECT_FLOAT_EQ(deferred->groupScale[0], 0.5f);
  EXPECT_FLOAT_EQ(deferred->groupScale[1], 0.5f);
  EXPECT_TRUE(deferred->relativeGroups);
  client::render::Pipeline pipeline{nullptr};
  const std::vector<client::render::ColorFormat> formats = pipeline.sceneColorFormats(&pack);
  const int gbufferCount = std::clamp(pack.definition.gbufferColorBuffers, 1, client::render::kMaxColorAttachments);
  const bool okMain = pack.colorTargets.ensure(854, 480, formats, gbufferCount);
  EXPECT_TRUE(okMain);
  EXPECT_TRUE(pipeline.ensureSceneTargets(&pack, 854, 480));
  EXPECT_EQ(::glGetError(), 0u);
}
TEST_F(RvoxDiag, DumpComplementaryFailingPrograms) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "ComplementaryReimagined_r5.8.1";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 PackInstance pack;
 pack.path = root;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
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
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *found->second);
 const std::filesystem::path outDir = std::filesystem::temp_directory_path() / "complementary_prepared";
 std::filesystem::create_directories(outDir);
 const auto dump = [&](const std::string& name) {
  const auto it = pack.definition.programs.find(name);
  if(it == pack.definition.programs.end()) {
   printf("[diag] program not found: %s\n", name.c_str());
   return;
  }
  const auto& spec = it->second;
  if(!spec.compute.empty()) {
   const std::string included = PackCompiler::resolveIncludes(pack, spec.compute);
   const std::string prepared =
       client::render::prepareSource(name, client::render::ShaderStage::Compute, pack.definition, included);
   std::ofstream(outDir / (name + ".csh.expanded")) << included;
   std::ofstream(outDir / (name + ".csh.prepared")) << prepared;
   printf("[diag] dumped %s compute (%zu -> %zu)\n", name.c_str(), included.size(), prepared.size());
   return;
  }
  const std::string vertexIncluded = PackCompiler::resolveIncludes(pack, spec.vertex);
  const std::string fragmentIncluded = PackCompiler::resolveIncludes(pack, spec.fragment);
  const std::string vertexPrepared =
      client::render::prepareSource(name, client::render::ShaderStage::Vertex, pack.definition, vertexIncluded);
  const std::string fragmentPrepared =
      client::render::prepareSource(name, client::render::ShaderStage::Fragment, pack.definition, fragmentIncluded);
  std::ofstream(outDir / (name + ".vsh.prepared")) << vertexPrepared;
  std::ofstream(outDir / (name + ".fsh.prepared")) << fragmentPrepared;
  printf("[diag] dumped %s (vertex %zu -> %zu, fragment %zu -> %zu)\n", name.c_str(), vertexIncluded.size(),
         vertexPrepared.size(), fragmentIncluded.size(), fragmentPrepared.size());
 };
 dump("shadowcomp#compute");
 dump("composite");
 printf("[diag] output dir: %s\n", outDir.string().c_str());
}
TEST_F(RvoxDiag, CompileAllComplementaryPrograms) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "ComplementaryReimagined_r5.8.1";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 PackInstance pack;
 pack.path = root;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
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
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *found->second);
 pack.programs = std::make_unique<client::gl::ProgramCache>(std::filesystem::path{});
 std::vector<std::string> failures;
 std::vector<std::string> names;
 for(const auto& [name, ignored] : pack.definition.programs) names.push_back(name);
 std::sort(names.begin(), names.end());
 for(const std::string& name : names) {
  const auto log = [&failures, &name](PackInstance&, const std::string& message,
                                      net::minecraft::util::logging::LogLevel) {
   failures.push_back(name + ": " + message);
  };
  if(!client::render::isProgramEnabledCached(pack.definition, pack.settings, name, pack.programEnabledCache)) {
   printf("[diag] skipped disabled %s\n", name.c_str());
   continue;
  }
  if(PackCompiler::compile(pack, name, log) == nullptr) {
   failures.push_back(name + ": returned null");
  }
 }
 printf("[diag] compiled %zu programs, %zu failures\n", names.size(), failures.size());
 for(const std::string& failure : failures) printf("[diag] %s\n", failure.c_str());
 for(const auto& [key, value] : pack.definition.programEnabled) {
  printf("[diag] programEnabled %s = %s\n", key.c_str(), value.c_str());
 }
 if(const auto it = pack.definition.programs.find("shadowcomp#compute"); it != pack.definition.programs.end()) {
  printf("[diag] shadowcomp compute path: %s\n", it->second.compute.c_str());
 }
 EXPECT_TRUE(failures.empty()) << join(failures);
}
TEST_F(RvoxDiag, CompileAllRenderPearlPrograms) {
 while(::glGetError() != 0) {}
 const std::filesystem::path root = shaderTestPack("RenderPearl v2.8.0-beta.4");
 ASSERT_TRUE(std::filesystem::is_directory(root));
 PackInstance pack;
 pack.path = root;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 ASSERT_FALSE(resources.empty());
 ASSERT_TRUE(PackLoader::load(
     resources,
     [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error))
     << pack.summary.error;
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 const std::set<std::string> required = {"BLOCK_EMISSION_ATTRIBUTE", "COMPUTE_SHADERS", "CUSTOM_IMAGES",
                                         "ENTITY_TRANSLUCENT", "PER_BUFFER_BLENDING",
                                         "SEPARATE_HARDWARE_SAMPLERS", "SSBO"};
 EXPECT_EQ(pack.rootDefinition.requiredFeatures, required);
 EXPECT_EQ(pack.rootDefinition.optionalFeatures, (std::set<std::string>{"FADE_VARIABLE"}));
 EXPECT_FALSE(pack.rootDefinition.oldHandLight);
 EXPECT_TRUE(pack.rootDefinition.allowConcurrentCompute);
 EXPECT_TRUE(pack.rootDefinition.separateEntityDraws);
 EXPECT_TRUE(pack.rootDefinition.separateAo);
 ASSERT_EQ(pack.rootDefinition.images.size(), 2u);
 EXPECT_EQ(pack.rootDefinition.images[0].name, "edge");
 EXPECT_EQ(pack.rootDefinition.images[0].sampler, "edgeS");
 EXPECT_EQ(pack.rootDefinition.images[1].name, "blendWeight");
 EXPECT_EQ(pack.rootDefinition.images[1].sampler, "blendWeightS");
 EXPECT_EQ(pack.rootDefinition.customTextures.size(), 3u);
 const auto bufferBytes = [&pack](int index) {
  const auto found = std::find_if(pack.rootDefinition.bufferObjects.begin(), pack.rootDefinition.bufferObjects.end(),
                                  [index](const auto& buffer) { return buffer.index == index; });
  return found == pack.rootDefinition.bufferObjects.end() ? std::size_t{} : found->byteSize;
 };
 EXPECT_EQ(bufferBytes(0), 8u);
 EXPECT_EQ(bufferBytes(1), 30736u);
 EXPECT_EQ(bufferBytes(2), 40976u);
 EXPECT_EQ(bufferBytes(3), 16u);
 EXPECT_EQ(bufferBytes(4), 8u);
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *found->second);
 for(const auto& [name, ignored] : pack.definition.programs) {
  (void)ignored;
  EXPECT_FALSE(name.rfind("dh_", 0) == 0) << name;
  EXPECT_FALSE(name.rfind("clrwl_", 0) == 0) << name;
  EXPECT_FALSE(name.rfind("voxy", 0) == 0) << name;
 }
 std::string runtimeError;
 ASSERT_TRUE(pack.rebuildRuntime(runtimeError)) << runtimeError;
 std::vector<std::string> failures;
 std::vector<std::string> names;
 for(const auto& [name, ignored] : pack.definition.programs) names.push_back(name);
 std::sort(names.begin(), names.end());
 for(const std::string& name : names) {
  if(!client::render::isProgramEnabledCached(pack.definition, pack.settings, name, pack.programEnabledCache)) {
   continue;
  }
  const auto log = [&failures, &name](PackInstance&, const std::string& message,
                                      net::minecraft::util::logging::LogLevel) {
   failures.push_back(name + ": " + message);
  };
  if(PackCompiler::compile(pack, name, log) == nullptr) failures.push_back(name + ": returned null");
 }
 EXPECT_TRUE(failures.empty()) << join(failures);
 ASSERT_TRUE(pack.buildExecutionPlan(runtimeError)) << runtimeError;
 for(const client::render::WorldProgramId id : {client::render::WorldProgramId::TerrainSolid,
                                                client::render::WorldProgramId::TerrainCutout,
                                                client::render::WorldProgramId::TerrainTranslucent,
                                                client::render::WorldProgramId::Entities,
                                                client::render::WorldProgramId::EntitiesTranslucent,
                                                client::render::WorldProgramId::Hand}) {
  EXPECT_NE(pack.worldPrograms[static_cast<std::size_t>(id)][0].program, nullptr)
      << client::render::worldProgramKey(id);
 }
 client::render::Pipeline pipeline{nullptr};
 const std::vector<client::render::ColorFormat> formats = pipeline.sceneColorFormats(&pack);
 ASSERT_GT(formats.size(), 2u);
 EXPECT_EQ(formats[0], client::render::ColorFormat::Rgba8);
 EXPECT_EQ(formats[1], client::render::ColorFormat::Rgba16F);
 EXPECT_EQ(formats[2], client::render::ColorFormat::Rgba32Ui);
 const int colorCount = std::clamp(pack.definition.gbufferColorBuffers, 1,
                                   client::render::kMaxColorAttachments);
 ASSERT_TRUE(pack.colorTargets.ensure(64, 64, formats, colorCount));
 ASSERT_TRUE(pipeline.ensureSceneTargets(&pack, 64, 64));
 ASSERT_TRUE(pipeline.preparePackResources(pack, 64, 64));
 client::gl::ShaderProgram* entityProgram =
     pack.worldPrograms[static_cast<std::size_t>(client::render::WorldProgramId::Entities)][0].program;
 ASSERT_NE(entityProgram, nullptr);
 ASSERT_EQ(entityProgram->drawBufferColortexIndices(), (std::vector<int>{1, 2}));
 std::vector<bool> clearEnabled(static_cast<std::size_t>(colorCount), true);
 std::vector<std::array<float, 4>> clearColors(static_cast<std::size_t>(colorCount));
 pack.colorTargets.clearColors(clearEnabled, clearColors);
 pack.colorTargets.bindGbuffers();
 entityProgram->bind();
 entityProgram->applyDrawBuffers(colorCount);
 client::render::PackUniformValues uniforms;
 uniforms.frameCounter = 1;
 uniforms.viewWidth = 64.0f;
 uniforms.viewHeight = 64.0f;
 uniforms.aspectRatio = 1.0f;
 uniforms.irisCurrentAlphaTest = 0.1f;
 uniforms.sunAngle = 0.25f;
 uniforms.sunIntensity = 1.0f;
 uniforms.skyColor[0] = 0.5f;
 uniforms.skyColor[1] = 0.7f;
 uniforms.skyColor[2] = 1.0f;
 uniforms.fogColor[0] = 0.5f;
 uniforms.fogColor[1] = 0.7f;
 uniforms.fogColor[2] = 1.0f;
 uniforms.fogStart = 192.0f;
 uniforms.fogEnd = 256.0f;
 pipeline.setFrameUniforms(uniforms, pack.definition, &pack);
 client::render::uploadShaderUniforms(*entityProgram, uniforms, true);
 client::render::uploadIdentityDrawMatrices(*entityProgram);
 pack.customUniforms.upload(*entityProgram);
 entityProgram->set1i("gtexture", 0);
 entityProgram->set1i("iris_overlay", 4);
 entityProgram->set1i("LLCollect", 0);
 client::render::core::activeTexture(client::gl::tex::Texture0);
 client::gl::GlTexture diffuse = solidTexture({255, 64, 32, 255});
 client::render::core::activeTexture(client::gl::tex::Texture0 + 4);
 client::gl::GlTexture overlay = solidTexture({0, 0, 0, 255});
 client::render::core::activeTexture(client::gl::tex::Texture0);
 std::array<client::render::TessellatorVertex, 3> vertices{};
 const std::array<std::array<float, 2>, 3> positions = {{{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}}};
 const std::array<std::array<float, 2>, 3> coordinates = {{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}}};
 for(std::size_t index = 0; index < vertices.size(); ++index) {
  auto& vertex = vertices[index];
  vertex.x = positions[index][0];
  vertex.y = positions[index][1];
  vertex.u = coordinates[index][0];
  vertex.v = coordinates[index][1];
  vertex.color = 0xFFFFFFFFu;
  vertex.normal = 0x007F0000;
  vertex.light = 0x00F00000;
  vertex.midU = 0.5f;
  vertex.midV = 0.5f;
  vertex.tangent[0] = 32767;
  vertex.tangent[3] = 32767;
  vertex.irisEntity[0] = -1;
  vertex.irisEntity[1] = -1;
  vertex.irisEntity[2] = -1;
 }
 unsigned int vaoHandle = 0;
 unsigned int vboHandle = 0;
 client::gl::GLCore::genVertexArrays(1, &vaoHandle);
 client::gl::GLCore::genBuffers(1, &vboHandle);
 client::gl::GlVao vao(vaoHandle);
 client::gl::GlBuffer vbo(vboHandle);
 client::gl::GLCore::bindVertexArray(vao.handle());
 client::gl::GLCore::bindBuffer(0x8892, vbo.handle());
 client::gl::GLCore::bufferData(0x8892, static_cast<intptr_t>(sizeof(vertices)), vertices.data(), 0x88E4);
 client::render::core::invalidateAttribCache();
 client::render::core::configureAttribs(vbo.handle(), 0, sizeof(client::render::TessellatorVertex), true, true);
 client::render::core::disableBlend();
 client::render::core::disableCull();
 ::glClearDepth(1.0);
 client::render::core::clear(client::gl::attrib::DepthBufferBit);
 client::render::core::depthTestWrite(true);
 client::render::core::viewport(0, 0, 64, 64);
 ::glDrawArrays(client::gl::prim::Triangles, 0, 3);
 std::array<float, 4> entityGbuffer{};
 ::glReadBuffer(0x8CE1);
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba, client::gl::pixel::Float,
                entityGbuffer.data());
 std::array<std::uint32_t, 4> entityMetadata{};
 ::glReadBuffer(0x8CE2);
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::RgbaInteger, client::gl::pixel::UnsignedInt,
                entityMetadata.data());
 EXPECT_GT(entityGbuffer[0], 0.5f);
 EXPECT_GT(entityGbuffer[1], 0.005f);
 EXPECT_GT(entityGbuffer[2], 0.001f);
 EXPECT_GT(entityGbuffer[3], -0.1f);
 EXPECT_LT(entityGbuffer[3], 0.1f);
 EXPECT_NE(entityMetadata[1], 0u);
 EXPECT_NE(entityMetadata[2] & 0x7FFFu, 0u);
 client::render::core::invalidateAttribCache();
 client::gl::GLCore::bindVertexArray(0);
 client::gl::ShaderProgram::unbind();
 pack.colorTargets.endGbuffers();
 client::gl::GlTexture shadowDepth = solidDepthTexture(1.0f);
 ASSERT_TRUE(pipeline.renderDeferred(&pack, static_cast<int>(shadowDepth.handle()),
                                     static_cast<int>(shadowDepth.handle()), nullptr, 0, nullptr));
 pack.colorTargets.bindGbuffers();
 std::array<float, 4> litEntity{};
 ::glReadBuffer(0x8CE1);
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba, client::gl::pixel::Float, litEntity.data());
 EXPECT_GT(litEntity[0] + litEntity[1] + litEntity[2], 0.01f);
 client::gl::ShaderProgram* waterProgram =
     pack.worldPrograms[static_cast<std::size_t>(client::render::WorldProgramId::TerrainTranslucent)][0].program;
 ASSERT_NE(waterProgram, nullptr);
 EXPECT_EQ(pack.worldPrograms[static_cast<std::size_t>(client::render::WorldProgramId::TerrainTranslucent)][0].resolvedKey,
           "gbuffers_water");
 EXPECT_FALSE(waterProgram->tessellation());
 EXPECT_EQ(client::gl::GLCore::getAttribLocation(waterProgram->handle(), "vaPosition"), 0);
 EXPECT_EQ(client::gl::GLCore::getAttribLocation(waterProgram->handle(), "vaUV0"), 1);
 EXPECT_EQ(client::gl::GLCore::getAttribLocation(waterProgram->handle(), "mc_Entity"), 6);
 EXPECT_EQ(client::gl::GLCore::getAttribLocation(waterProgram->handle(), "mc_chunkFade"), 12);
 net::minecraft::util::math::Matrix4f waterProjection;
 std::fill(std::begin(waterProjection.m), std::end(waterProjection.m), 0.0f);
 waterProjection.m[0] = 0.5f;
 waterProjection.m[5] = 0.5f;
 waterProjection.m[10] = -1.0f;
 waterProjection.m[11] = -1.0f;
 waterProjection.m[14] = -0.2f;
 std::copy(std::begin(waterProjection.m), std::end(waterProjection.m), uniforms.gbufferProjection);
 net::minecraft::util::math::Matrix4f waterProjectionInverse = waterProjection;
 waterProjectionInverse.invert();
 std::copy(std::begin(waterProjectionInverse.m), std::end(waterProjectionInverse.m),
           uniforms.gbufferProjectionInverse);
 pipeline.setFrameUniforms(uniforms, pack.definition, &pack);
 client::render::core::FogUniforms waterFog;
 waterFog.enabled = true;
 waterFog.mode = 1;
 waterFog.start = uniforms.fogStart;
 waterFog.end = uniforms.fogEnd;
 waterFog.color[0] = uniforms.fogColor[0];
 waterFog.color[1] = uniforms.fogColor[1];
 waterFog.color[2] = uniforms.fogColor[2];
 client::render::core::setFog(waterFog);
 for(auto& vertex : vertices) {
  vertex.z = -0.5f;
  vertex.color = 0xFFFFFFFFu;
  vertex.normal = 0x007F0000;
  vertex.midBlock = 0;
  vertex.light = 0x00F00000;
  vertex.entity[0] = -1;
  vertex.entity[1] = 1;
  vertex.entity[2] = 0;
  vertex.entity[3] = 0;
 }
 client::gl::GLCore::bindVertexArray(vao.handle());
 client::gl::GLCore::bindBuffer(0x8892, vbo.handle());
 client::gl::GLCore::bufferData(0x8892, static_cast<intptr_t>(sizeof(vertices)), vertices.data(), 0x88E4);
 client::render::core::invalidateAttribCache();
 client::render::core::configureAttribs(vbo.handle(), 0, sizeof(client::render::TessellatorVertex), true, true);
 waterProgram->bind();
 waterProgram->applyDrawBuffers(colorCount);
 uniforms.irisCurrentAlphaTest = 0.0001f;
 client::render::uploadShaderUniforms(*waterProgram, uniforms, true);
 client::render::uploadIdentityDrawMatrices(*waterProgram);
 pack.customUniforms.upload(*waterProgram);
 waterProgram->set1i("gtexture", 0);
 waterProgram->set1i("LLCollect", 0);
 waterProgram->set3f("chunkOffset", 0.0f, 0.0f, 0.0f);
 waterProgram->set3f("waveState", 0.0f, 0.0f, 0.0f);
 client::render::core::activeTexture(client::gl::tex::Texture0);
 client::gl::GlTexture waterTexture = solidTexture({16, 96, 255, 255});
 client::render::applyBufferBlends(pack.definition, "gbuffers_water",
                                   waterProgram->drawBufferColortexIndices());
 ASSERT_EQ(waterProgram->drawBufferColortexIndices(), (std::vector<int>{1}));
 int activeProgram = 0;
 int drawBuffer = 0;
 ::glGetIntegerv(0x8B8D, &activeProgram);
 ::glGetIntegerv(0x8825, &drawBuffer);
 ASSERT_EQ(activeProgram, static_cast<int>(waterProgram->handle()));
 ASSERT_EQ(drawBuffer, 0x8CE1);
 client::render::core::disableCull();
 client::render::core::colorMask(true, true, true, true);
 ::glDisable(client::gl::cap::ScissorTest);
 client::render::core::setAlphaTestRef(0.0001f);
 client::render::core::depthTestWrite(true);
 client::render::core::clearDepth(1.0);
 client::render::core::clear(client::gl::attrib::DepthBufferBit);
 client::render::core::depthTestWrite(false);
 client::render::core::RenderPass waterPass;
 waterPass.buffer = vbo.handle();
 waterPass.vertexCount = vertices.size();
 waterPass.stride = sizeof(client::render::TessellatorVertex);
 waterPass.hasTexture = true;
 waterPass.hasNormals = true;
 waterPass.programOverride = waterProgram;
 waterPass.projection = waterProjection;
 EXPECT_EQ(pack.preparedSourceCache.at("gbuffers_water:0").find(
               "immut vec4 clip = proj_mmul(mat4(projectionMatrix), view);"),
           std::string::npos);
 unsigned int samplesQuery = 0;
 client::gl::GLCore::genQueries(1, &samplesQuery);
 client::gl::GLCore::beginQuery(0x8C2F, samplesQuery);
 client::render::core::submit(waterPass);
 client::gl::GLCore::endQuery(0x8C2F);
 int samplesPassed = 0;
 client::gl::GLCore::getQueryObjectiv(samplesQuery, 0x8866, &samplesPassed);
 client::gl::GLCore::deleteQueries(1, &samplesQuery);
 std::array<float, 4> waterPixel{};
 ::glReadBuffer(0x8CE1);
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba, client::gl::pixel::Float, waterPixel.data());
 const float waterDelta = std::abs(waterPixel[0] - litEntity[0]) + std::abs(waterPixel[1] - litEntity[1]) +
                          std::abs(waterPixel[2] - litEntity[2]);
 client::render::core::lockBlend(nullptr);
 client::render::core::submit(waterPass);
 std::array<float, 4> rawWater{};
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba, client::gl::pixel::Float, rawWater.data());
 EXPECT_GT(waterDelta, 0.001f) << samplesPassed << ' ' << waterPixel[3] << ' ' << rawWater[0] << ' '
                               << rawWater[1] << ' ' << rawWater[2] << ' ' << rawWater[3];
 EXPECT_GT(samplesPassed, 0);
 EXPECT_GT(rawWater[0] + rawWater[1] + rawWater[2], 0.001f);
 EXPECT_GT(rawWater[3], 0.0f);
 client::render::core::unlockBlend();
 client::render::core::depthTestWrite(true);
 client::render::core::invalidateAttribCache();
 client::gl::GLCore::bindVertexArray(0);
 client::gl::ShaderProgram::unbind();
 pack.colorTargets.endGbuffers();
 client::render::core::activeTexture(client::gl::tex::Texture0);
 EXPECT_EQ(::glGetError(), 0u);
}
TEST_F(RvoxDiag, DumpComputeSources) {
 const std::filesystem::path zipPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9.zip";
 const std::filesystem::path dirPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9";
 PackInstance pack;
 if(std::filesystem::exists(zipPath)) {
  pack.path = zipPath;
  pack.directory = false;
  pack.zip = std::make_unique<client::resource::pack::ZippedTexturePack>(zipPath);
  pack.zip->open();
  const std::vector<std::string> resources = zipResources(*pack.zip);
  ASSERT_TRUE(PackLoader::load(
      resources,
      [&pack](std::string_view path) { return PackCompiler::cachedText(pack, std::string(path)); },
      pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
 } else {
  ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << dirPath.string();
  pack.path = dirPath;
  pack.directory = true;
  const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dirPath);
  ASSERT_TRUE(PackLoader::load(
      resources,
      [&pack](std::string_view path) { return PackCompiler::readText(pack, std::string(path)); },
      pack.definition, pack.sourceOptions, pack.summary.error))
      << pack.summary.error;
 }
 pack.summary.valid = true;
 pack.rootDefinition = pack.definition;
 for(const PackSetting& setting : pack.rootDefinition.settings) {
  pack.settings.emplace(setting.key, setting.defaultValue);
 }
 const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
 ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
 mergeDimension(pack.definition, *found->second);
 const std::filesystem::path outDir = std::filesystem::temp_directory_path() / "rvox_prepared";
 std::filesystem::create_directories(outDir);
 for(const char* name : {"deferred1#compute", "shadowcomp1_b#compute"}) {
  const auto it = pack.definition.programs.find(name);
  if(it == pack.definition.programs.end()) {
   printf("[diag] program not found: %s\n", name);
   continue;
  }
  const std::string computeIncluded = PackCompiler::resolveIncludes(pack, it->second.compute);
  const std::string computePrepared =
      client::render::prepareSource(name, client::render::ShaderStage::Compute, pack.definition, computeIncluded);
  std::ofstream(outDir / (std::string(name) + ".csh.expanded")) << computeIncluded;
  std::ofstream(outDir / (std::string(name) + ".csh.prepared")) << computePrepared;
  printf("[diag] dumped %s (%zu -> %zu)\n", name, computeIncluded.size(), computePrepared.size());
 }
 printf("[diag] output dir: %s\n", outDir.string().c_str());
}
} // namespace net::minecraft::test
