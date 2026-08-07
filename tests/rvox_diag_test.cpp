#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
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
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
  glfwTerminate();
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
} // namespace
TEST_F(RvoxDiag, DumpPreparedSources) {
 const std::filesystem::path zipPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9.zip";
 ASSERT_TRUE(std::filesystem::exists(zipPath)) << zipPath.string();
 PackInstance pack;
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
  ASSERT_TRUE(std::filesystem::exists(zipPath)) << zipPath.string();
  PackInstance pack;
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
  pack.summary.valid = true;
  pack.rootDefinition = pack.definition;
  for(const PackSetting& setting : pack.rootDefinition.settings) {
   pack.settings.emplace(setting.key, setting.defaultValue);
  }
  const auto found = pack.rootDefinition.dimensionDefinitions.find("*");
  ASSERT_NE(found, pack.rootDefinition.dimensionDefinitions.end());
  mergeDimension(pack.definition, *found->second);
  pack.programs = std::make_unique<client::gl::ProgramCache>();
  std::vector<std::string> failures;
  for(const auto& [name, ignored] : pack.definition.programs) {
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
 pack.programs = std::make_unique<client::gl::ProgramCache>();
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
TEST_F(RvoxDiag, DumpComputeSources) { const std::filesystem::path zipPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9.zip";
 ASSERT_TRUE(std::filesystem::exists(zipPath)) << zipPath.string();
 PackInstance pack;
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
