// Complementary's profile buttons must land every option the profile lists.
//
// shaders.properties ships pairs that the pack itself validates at runtime:
//
//   profile.ULTRA = ... shadowDistance=256.0 ... COLORED_LIGHTING=512 ...
//   program/final.glsl: if (COLORED_LIGHTING_INTERNAL > shadowDistance*2) -> error text
//
// ULTRA sits exactly on that boundary (512 vs 256*2), so a preset that applies
// COLORED_LIGHTING while shadowDistance drifts by one step paints the pack's own
// "Advanced Color Tracing must not be set higher than the Shadow Distance
// setting" screen the moment the button is pressed.
#include <gtest/gtest.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::PackProfile;
using client::render::PackSetting;
using client::render::normalizeSettingValue;
constexpr const char* kPack = "ComplementaryReimagined_r5.8.1";
bool loadPack(std::string_view name,
              PackInstance& pack,
              const std::unordered_map<std::string, std::string>& optionValues = {}) {
 const std::filesystem::path dir = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / name;
 if(!std::filesystem::is_directory(dir)) return false;
 pack.path = dir;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dir);
 return PackLoader::load(
     resources,
     [&pack](std::string_view path) { return client::render::PackCompiler::readText(pack, std::string(path)); },
     pack.definition, pack.sourceOptions, pack.summary.error, optionValues);
}
const PackSetting* findSetting(const PackDefinition& definition, const std::string& key) {
 for(const PackSetting& setting : definition.settings)
  if(setting.key == key) return &setting;
 return nullptr;
}
const PackProfile* findProfile(const PackDefinition& definition, const std::string& name) {
 for(const PackProfile& profile : definition.profiles)
  if(profile.name == name) return &profile;
 return nullptr;
}
// Mirrors Pipeline::setSettings for a profile-button click: the screen hands it
// the preset's key/value pairs and nothing else.
std::unordered_map<std::string, std::string> applyProfile(const PackDefinition& definition,
                                                          const PackProfile& profile) {
 std::unordered_map<std::string, std::string> merged;
 for(const PackSetting& setting : definition.settings) merged[setting.key] = setting.defaultValue;
 for(const auto& [key, value] : profile.values) {
  for(const PackSetting& setting : definition.settings) {
   if(setting.key != key) continue;
   std::string normalized;
   if(!normalizeSettingValue(setting, value, normalized)) break;
   merged[key] = std::move(normalized);
   break;
  }
 }
 return merged;
}
std::string emittedLine(const std::string& source, const std::string& needle) {
 const std::size_t at = source.find(needle);
 if(at == std::string::npos) return {};
 const std::size_t start = source.rfind('\n', at);
 const std::size_t end = source.find('\n', at);
 return source.substr(start == std::string::npos ? 0 : start + 1,
                      end == std::string::npos ? std::string::npos : end - (start + 1));
}
class ComplementaryProfiles : public ::testing::Test {
 protected:
 void SetUp() override {
  if(!loadPack(kPack, pack_)) GTEST_SKIP() << "Complementary is not installed in shaders/: " << pack_.summary.error;
 }
 PackInstance pack_;
};
// A const directive's default has to stay the pack's own token. Reformatting it
// through std::to_string turned `192.0` into `192.000000`, which matches nothing
// in the //[...] list - so the loader appended it as an extra slider stop and
// writeSettingsFile persisted the stock value as if the user had changed it.
TEST_F(ComplementaryProfiles, ConstDirectiveDefaultIsThePacksOwnToken) {
 const PackSetting* setting = findSetting(pack_.definition, "shadowDistance");
 ASSERT_NE(setting, nullptr) << "shadowDistance never became a setting, so no profile and no slider can move it";
 EXPECT_TRUE(setting->asSlider) << "shaders.properties lists shadowDistance under sliders=";
 EXPECT_EQ(setting->defaultValue, "192.0");
 EXPECT_NE(std::find(setting->valueOrder.begin(), setting->valueOrder.end(), setting->defaultValue),
           setting->valueOrder.end())
     << "the default must be one of the declared stops, not a synthetic extra one";
 std::vector<std::string> sorted = setting->valueOrder;
 std::sort(sorted.begin(), sorted.end());
 EXPECT_EQ(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end())
     << "duplicate stop in the slider's value list";
}
// Iris has no `profile` option - ProfileSet.scan derives the active profile by
// matching current values, and clicking one applies its option values on top of
// what is already set. A synthetic enum invented an option the pack never
// declared, and nothing could ever set it.
TEST_F(ComplementaryProfiles, NoSyntheticProfileOptionIsInvented) {
 EXPECT_EQ(findSetting(pack_.definition, "profile"), nullptr);
 EXPECT_FALSE(pack_.definition.profiles.empty()) << "the pack's own profile list must still parse";
}
TEST_F(ComplementaryProfiles, PresetsSatisfyThePacksOwnActVsShadowDistanceGuard) {
 for(const char* name : {"POTATO", "VERYLOW", "LOW", "MEDIUM", "HIGH", "VERYHIGH", "ULTRA"}) {
  const PackProfile* profile = findProfile(pack_.definition, name);
  ASSERT_NE(profile, nullptr) << name << " is missing from the parsed profiles";
  const std::unordered_map<std::string, std::string> merged = applyProfile(pack_.definition, *profile);
  PackInstance applied;
  ASSERT_TRUE(loadPack(kPack, applied, merged)) << applied.summary.error;
  applied.settings = merged;
  // The text the driver actually sees: dimension.world0=* so every world runs
  // world0/final.fsh, which pulls /program/final.glsl and /lib/common.glsl in
  // through the same dimension-prefixed include resolution the pipeline uses.
  const std::string source = client::render::PackCompiler::resolveIncludes(applied, "shaders/world0/final.fsh");
  const double act = std::strtod(merged.at("COLORED_LIGHTING").c_str(), nullptr);
  const double distance = std::strtod(merged.at("shadowDistance").c_str(), nullptr);
  EXPECT_EQ(emittedLine(source, "#define COLORED_LIGHTING ").rfind("#define COLORED_LIGHTING " +
                                                                   merged.at("COLORED_LIGHTING") + " ", 0),
            0u)
      << name;
  EXPECT_NE(emittedLine(source, "const float shadowDistance")
                .find("= " + merged.at("shadowDistance") + ";"),
            std::string::npos)
      << name << ": the const directive did not carry the value the profile set";
  EXPECT_LE(act, distance * 2.0) << name << " trips final.glsl's own error screen";
  EXPECT_EQ(applied.definition.shadowDistance, static_cast<float>(distance)) << name;
 }
}
// includedSourceCache and preparedSourceCache hold source that rewriteOptions
// has already baked the option values into, and neither key involves content.
// rebuildRuntime is the only thing that empties them, so every settings change
// has to run it or the recompile rebuilds the shader the options were just
// changed away from.
TEST_F(ComplementaryProfiles, RebuildRuntimeInvalidatesTheOptionBakedSource) {
 const std::string before = client::render::PackCompiler::resolveIncludes(pack_, "shaders/world0/final.fsh");
 ASSERT_NE(before.find("#define COLORED_LIGHTING 0 "), std::string::npos)
     << "expected the stock value to be baked into the resolved source";
 pack_.settings["COLORED_LIGHTING"] = "512";
 pack_.settings["shadowDistance"] = "256.0";
 std::string error;
 pack_.rebuildRuntime(error);
 const std::string after = client::render::PackCompiler::resolveIncludes(pack_, "shaders/world0/final.fsh");
 EXPECT_NE(after.find("#define COLORED_LIGHTING 512 "), std::string::npos)
     << "resolveIncludes served the cached pre-change source";
 EXPECT_NE(after.find("const float shadowDistance = 256.0;"), std::string::npos);
}
// The loader half is only half the click. Pipeline::setSettings merges the
// preset over the pack's live settings, writes shaders/<pack>.txt, and the
// screen calls poll() straight afterwards - so this drives the real object.
class PipelineProfileClick : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "profile-preset", nullptr, nullptr);
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
 void TearDown() override {
  std::error_code ec;
  std::filesystem::remove(settingsFile(), ec);
 }
 static std::filesystem::path settingsFile() {
  return std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / (std::string(kPack) + ".txt");
 }
 static GLFWwindow* window_;
};
GLFWwindow* PipelineProfileClick::window_ = nullptr;
TEST_F(PipelineProfileClick, EveryPresetLandsBothActAndShadowDistance) {
 client::render::Pipeline pipeline(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR), nullptr,
                                   std::filesystem::temp_directory_path() / "profile-preset-cache");
 if(!pipeline.select(kPack)) GTEST_SKIP() << "Complementary is not installed in shaders/";
 ASSERT_NE(pipeline.selectedDefinition(), nullptr);
 for(const char* name : {"VERYHIGH", "ULTRA", "HIGH", "ULTRA"}) {
  // setSettings replaces the definition, so the profile has to be copied out
  // before the click - exactly what ShaderpackScreen's button lambda does.
  const PackProfile* found = findProfile(*pipeline.selectedDefinition(), name);
  ASSERT_NE(found, nullptr) << name;
  const PackProfile profile = *found;
  std::vector<std::pair<std::string, std::string>> values;
  for(const auto& [key, value] : profile.values) values.emplace_back(key, value);
  pipeline.setSettings(values);
  EXPECT_EQ(pipeline.settingValue("COLORED_LIGHTING"), profile.values.at("COLORED_LIGHTING")) << name;
  EXPECT_EQ(pipeline.settingValue("shadowDistance"), profile.values.at("shadowDistance")) << name;
  // setSettings persisted into the watched shaders/ directory; the poll() that
  // ShaderpackScreen::rebuildLayout does right after the click must not see the
  // sidecar as a pack change, but the values still have to survive a real one.
  pipeline.reload();
  ASSERT_TRUE(pipeline.select(kPack));
  const std::string act = pipeline.settingValue("COLORED_LIGHTING");
  const std::string distance = pipeline.settingValue("shadowDistance");
  EXPECT_EQ(act, profile.values.at("COLORED_LIGHTING")) << name;
  EXPECT_EQ(distance, profile.values.at("shadowDistance")) << name;
  EXPECT_LE(std::strtod(act.c_str(), nullptr), std::strtod(distance.c_str(), nullptr) * 2.0)
      << "clicking " << name << " leaves final.glsl's guard tripped";
 }
}
// HIGH is the pack's own stock configuration - every value it sets is already
// the default - so clicking it from stock must leave nothing to persist. It
// only wrote a file when the loader reformatted `192.0` into `192.000000` and
// then failed to recognise its own default.
TEST_F(PipelineProfileClick, ClickingTheStockPresetPersistsNothing) {
 client::render::Pipeline pipeline(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR), nullptr,
                                   std::filesystem::temp_directory_path() / "profile-preset-cache");
 if(!pipeline.select(kPack)) GTEST_SKIP() << "Complementary is not installed in shaders/";
 const PackProfile* high = findProfile(*pipeline.selectedDefinition(), "HIGH");
 ASSERT_NE(high, nullptr);
 std::vector<std::pair<std::string, std::string>> values;
 for(const auto& [key, value] : high->values) values.emplace_back(key, value);
 pipeline.setSettings(values);
 EXPECT_FALSE(std::filesystem::exists(settingsFile()))
     << "stock values were written as if the user had changed them";
 const PackProfile* ultra = findProfile(*pipeline.selectedDefinition(), "ULTRA");
 ASSERT_NE(ultra, nullptr);
 values.clear();
 for(const auto& [key, value] : ultra->values) values.emplace_back(key, value);
 pipeline.setSettings(values);
 ASSERT_TRUE(std::filesystem::exists(settingsFile()));
 bool wroteShadowDistance = false;
 std::ifstream in(settingsFile());
 for(std::string line; std::getline(in, line);) {
  if(line == "shadowDistance=256.0") wroteShadowDistance = true;
  EXPECT_EQ(line.rfind("profile=", 0), std::string::npos) << "the synthetic profile option is back";
 }
 EXPECT_TRUE(wroteShadowDistance) << "a genuinely changed const directive must persist";
}
} // namespace
} // namespace net::minecraft::test
