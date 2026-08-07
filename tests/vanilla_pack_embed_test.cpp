#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
namespace net::minecraft::client::render {
namespace {
std::vector<std::string> diskResources() {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "src" / "net" / "minecraft" /
     "client" / "render" / "shaders" / "glsl" / "vanilla";
 std::vector<std::string> result;
 std::error_code ec;
 for(std::filesystem::recursive_directory_iterator it(root, ec), end;
     !ec && it != end; it.increment(ec)) {
  if(!it->is_regular_file()) {
   continue;
  }
  result.push_back(std::filesystem::relative(it->path(), root, ec).generic_string());
 }
 std::sort(result.begin(), result.end());
 return result;
}
bool isShaderStage(const std::string& path) {
 const std::filesystem::path ext = std::filesystem::path(path).extension();
 return ext == ".vsh" || ext == ".fsh" || ext == ".gsh" || ext == ".csh" ||
        ext == ".tcs" || ext == ".tes" || ext == ".glsl";
}
} // namespace
// Guard against glsl/vanilla losing files: every pack file must be embedded.
// Shader stages are baked minified (no // comments, no #include lines, no blank
// lines), so only their shape is asserted here rather than byte equality.
TEST(VanillaPackEmbedTest, EmbedsEveryVanillaPackFile) {
 const std::vector<std::string> disk = diskResources();
 ASSERT_FALSE(disk.empty());
 EXPECT_EQ(VanillaPackEmbed::resources(), disk);
 for(const std::string& path : disk) {
  EXPECT_TRUE(VanillaPackEmbed::has(path)) << "glsl/vanilla/" << path << " is not embedded";
  EXPECT_FALSE(VanillaPackEmbed::get(path).empty()) << path;
 }
}
TEST(VanillaPackEmbedTest, BakedShadersAreMinifiedAndResolved) {
 for(const std::string& path : VanillaPackEmbed::resources()) {
  if(!isShaderStage(path)) {
   continue;
  }
  const std::string source = VanillaPackEmbed::get(path);
  EXPECT_EQ(source.find("//"), std::string::npos) << path << " still carries // comments";
  EXPECT_EQ(source.find("#include"), std::string::npos) << path << " still carries #include";
  EXPECT_EQ(source.find("\n\n"), std::string::npos) << path << " still carries blank lines";
 }
 // common.glsl content must be inlined into the stages that include it, not left
 // as an #include to resolve at runtime.
 EXPECT_NE(VanillaPackEmbed::get("shaders/gbuffers_terrain.fsh").find("faceShade"),
           std::string::npos);
}
// The embedded sources must be a self-sufficient pack: loading it without any
// on-disk shaders/vanilla directory has to produce a valid vanilla definition,
// and the /* RENDERTARGETS */ directives must have survived the bake.
TEST(VanillaPackEmbedTest, LoadsVanillaPackFromEmbeddedSources) {
 const std::vector<std::string> resources = VanillaPackEmbed::resources();
 ASSERT_FALSE(resources.empty());
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 ASSERT_TRUE(PackLoader::load(resources,
                              [](std::string_view path) { return VanillaPackEmbed::get(path); },
                              pack,
                              options,
                              error))
     << error;
 EXPECT_TRUE(pack.programs.contains("gbuffers_terrain"));
 EXPECT_TRUE(pack.programs.contains("final"));
 EXPECT_EQ(pack.gbufferColorBuffers, 1);
}
} // namespace net::minecraft::client::render
