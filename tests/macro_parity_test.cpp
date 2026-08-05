#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "support/glsl_snippets_test_fixture.hpp"
namespace net::minecraft::test {
namespace {
// Java StandardMacros.createStandardEnvironmentDefines emits this textual set
// (third_party/iris/.../gl/shader/StandardMacros.java). Empty value = bare flag.
// IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS / IRIS_TAG_SUPPORT / the translucency-sorting
// flag are defined even though the engine does not implement those capabilities
// (user decision Q6); MAX_COLOR_BUFFERS is hardware-derived so only presence is
// asserted (checked separately).
const std::unordered_map<std::string, std::string> kJavaMacroSet = {
    {"IS_IRIS", ""},
    {"IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS", ""},
    {"IRIS_HAS_TRANSLUCENCY_SORTING", ""},
    {"IRIS_TAG_SUPPORT", "2"},
    {"MC_NORMAL_MAP", ""},
    {"MC_SPECULAR_MAP", ""},
    {"MC_RENDER_QUALITY", "1.0"},
    {"MC_SHADOW_QUALITY", "1.0"}};

void expectDefine(const std::string& preamble, const std::string& name, const std::string& value) {
 const std::string needle =
     value.empty() ? "#define " + name + "\n" : "#define " + name + " " + value + "\n";
 EXPECT_NE(preamble.find(needle), std::string::npos) << "missing #define " << name << " " << value;
}
TEST(MacroParity, VersionPreambleMatchesJavaTextualMacroSet) {
 installTestGlslSnippets();
 const client::render::PackDefinition pack;
 const std::string preamble = client::render::versionPreamble(pack, "void main(){}");
 for(const auto& [name, value] : kJavaMacroSet) {
  expectDefine(preamble, name, value);
 }
 EXPECT_NE(preamble.find("#define MAX_COLOR_BUFFERS "), std::string::npos);
 EXPECT_NE(preamble.find("#define IS_IRIS\n"), std::string::npos);
}
TEST(MacroParity, CategoryDefinesReachJavaBiomeCategoriesCount) {
 installTestGlslSnippets();
 const client::render::PackDefinition pack;
 const std::string preamble = client::render::versionPreamble(pack, "void main(){}");
 // BiomeCategories.java has 19 entries (MOUNTAIN/UNDERGROUND appended after
 // NETHER); appendIndexedDefines emits CAT_<name> <ordinal>.
 expectDefine(preamble, "CAT_NETHER", "16");
 expectDefine(preamble, "CAT_MOUNTAIN", "17");
 expectDefine(preamble, "CAT_UNDERGROUND", "18");
 EXPECT_NE(preamble.find("#define CAT_NONE 0\n"), std::string::npos);
}
TEST(MacroParity, PreprocessorSeedMirrorsVersionPreamble) {
 client::render::PPMacroTable macros;
 client::render::seedMacrosFromDefines("", macros);
 EXPECT_TRUE(macros.contains("IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS"));
 EXPECT_TRUE(macros.contains("IRIS_HAS_TRANSLUCENCY_SORTING"));
 EXPECT_TRUE(macros.contains("MC_NORMAL_MAP"));
 EXPECT_TRUE(macros.contains("MC_SPECULAR_MAP"));
 EXPECT_EQ(macros["MC_NORMAL_MAP"].body, "1");
 EXPECT_EQ(macros["MC_SPECULAR_MAP"].body, "1");
 EXPECT_EQ(macros["MC_RENDER_QUALITY"].body, "1.0");
 EXPECT_EQ(macros["MC_SHADOW_QUALITY"].body, "1.0");
 EXPECT_EQ(macros["IRIS_TAG_SUPPORT"].body, "2");
 EXPECT_TRUE(client::render::evaluateIfExpression(
     "defined(MC_NORMAL_MAP) && MC_RENDER_QUALITY == 1.0 && IRIS_TAG_SUPPORT == 2", macros));
}
TEST(MacroParity, PreprocessorSeedPicksUpPreambleText) {
 client::render::PPMacroTable macros;
 client::render::seedMacrosFromDefines("#define MC_NORMAL_MAP\n#define MC_RENDER_QUALITY 1.0\n", macros);
 EXPECT_TRUE(macros.contains("MC_NORMAL_MAP"));
 EXPECT_EQ(macros["MC_RENDER_QUALITY"].body, "1.0");
}
void writeCacheEntry(const std::filesystem::path& root, std::uint64_t hash, std::uint32_t version) {
 char name[32]{};
 std::snprintf(name, sizeof(name), "%016llx.bin", static_cast<unsigned long long>(hash));
 std::error_code ec;
 std::filesystem::create_directories(root, ec);
 std::ofstream out(root / name, std::ios::binary | std::ios::trunc);
 const char magic[8] = {'M', 'C', 'S', 'P', 'B', 'I', 'N', '1'};
 const std::uint32_t format = 0x8B92;
 const std::uint32_t flags = 0;
 const std::uint32_t size = 4;
 const char bytes[4] = {1, 2, 3, 4};
 out.write(magic, 8);
 out.write(reinterpret_cast<const char*>(&version), sizeof(version));
 out.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
 out.write(reinterpret_cast<const char*>(&format), sizeof(format));
 out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
 out.write(reinterpret_cast<const char*>(&size), sizeof(size));
 out.write(bytes, static_cast<std::streamsize>(size));
}
TEST(MacroParity, CacheFormatVersionIsBumpedPastLegacy) {
 const std::filesystem::path root =
     std::filesystem::temp_directory_path() / "minecraft_shader_cache_parity_test";
 std::error_code ec;
 std::filesystem::remove_all(root, ec);
 const std::uint64_t hash = 0x1234ABCDULL;
 writeCacheEntry(root, hash, 2);
 {
  const client::gl::ShaderBinaryCache cache(root);
  EXPECT_FALSE(cache.tryLoad(hash).has_value()) << "legacy format-2 entry must be rejected";
 }
 writeCacheEntry(root, hash, 4);
 {
  const client::gl::ShaderBinaryCache cache(root);
  const auto blob = cache.tryLoad(hash);
  ASSERT_TRUE(blob.has_value());
  EXPECT_EQ(blob->contentHash, hash);
 }
 std::filesystem::remove_all(root, ec);
}
TEST(MacroParity, CacheRoundTripUsesCurrentFormat) {
 const std::filesystem::path root =
     std::filesystem::temp_directory_path() / "minecraft_shader_cache_parity_test_roundtrip";
 std::error_code ec;
 std::filesystem::remove_all(root, ec);
 client::gl::ShaderBinaryCache cache(root);
 client::gl::ProgramBinaryBlob blob;
 blob.contentHash = 0xDEADBEEFULL;
 blob.binaryFormat = 0x8B92;
 blob.flags = 0;
 blob.compute = false;
 blob.tessellation = false;
 blob.bytes = {1, 2, 3, 4};
 ASSERT_TRUE(cache.store(blob));
 const auto loaded = cache.tryLoad(blob.contentHash);
 ASSERT_TRUE(loaded.has_value());
 EXPECT_EQ(loaded->contentHash, blob.contentHash);
 EXPECT_EQ(loaded->binaryFormat, blob.binaryFormat);
 EXPECT_EQ(loaded->bytes, blob.bytes);
 std::filesystem::remove_all(root, ec);
}
} // namespace
} // namespace net::minecraft::test
