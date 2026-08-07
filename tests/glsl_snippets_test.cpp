#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "support/glsl_snippets_test_fixture.hpp"
namespace net::minecraft::test {
namespace {
using client::render::GlslSnippets;
TEST(GlslSnippetsTest, InjectedMapReturnsContent) {
 GlslSnippets::setSourceMapForTesting({{"probe", "void probe() {}\n"}});
 EXPECT_EQ(GlslSnippets::get("probe"), "void probe() {}\n");
}
TEST(GlslSnippetsTest, MissingNameReturnsEmptyAndCaches) {
 GlslSnippets::setSourceMapForTesting({{"probe", "present"}});
 EXPECT_TRUE(GlslSnippets::get("probe_unknown").empty());
 EXPECT_TRUE(GlslSnippets::get("probe_unknown").empty());
}
TEST(GlslSnippetsTest, ReinstalledMapClearsStaleCache) {
 GlslSnippets::setSourceMapForTesting({{"probe", "first"}});
 EXPECT_EQ(GlslSnippets::get("probe"), "first");
 GlslSnippets::setSourceMapForTesting({{"probe", "second"}});
 EXPECT_EQ(GlslSnippets::get("probe"), "second");
}
TEST(GlslSnippetsTest, EmptyMapRestoresEmbeddedMode) {
 GlslSnippets::setSourceMapForTesting({{"probe", "first"}});
 EXPECT_EQ(GlslSnippets::get("probe"), "first");
 GlslSnippets::setSourceMapForTesting({});
 // Back to embedded mode: a name that can never ship as an engine snippet
 // must come back empty without resurrecting the injected value.
 EXPECT_TRUE(GlslSnippets::get("probe").empty());
}
TEST(GlslSnippetsTest, SnippetFilesShippedAndComplete) {
 // Guard against src/.../shaders/glsl/ losing files: every engine snippet name
 // must resolve to non-empty text in the repo's own source tree.
 const std::unordered_map<std::string, std::string>& snippets = testGlslSnippets();
 EXPECT_FALSE(snippets.empty());
 for(const auto& [name, text] : snippets) {
  EXPECT_FALSE(text.empty()) << "src/.../shaders/glsl/" << name << ".glsl is missing or empty";
 }
}
TEST(GlslSnippetsTest, EmbeddedMapMatchesSourceFiles) {
 // The executable's embedded table must agree byte-for-byte with the source
 // .glsl files (the same input the build embeds), so a snippet added to
 // src/.../shaders/glsl/ but not re-embedded is caught here.
 const std::unordered_map<std::string, std::string>& source = testGlslSnippets();
 const std::unordered_map<std::string, std::string>& embedded = GlslSnippets::embeddedGlslSnippets();
 EXPECT_EQ(embedded.size(), source.size());
 for(const auto& [name, text] : source) {
  const auto found = embedded.find(name);
  ASSERT_NE(found, embedded.end()) << "embedded table missing snippet '" << name << "'";
  EXPECT_EQ(found->second, text) << "embedded text differs from source for '" << name << "'";
 }
}
TEST(GlslSnippetsTest, EngineSnippetsPreserveOriginalText) {
  // Spot-check the byte-exact text of snippets the transform tests assert on;
  // these must match the strings that used to live in the .cpp files.
 const std::unordered_map<std::string, std::string>& snippets = testGlslSnippets();
 EXPECT_EQ(snippets.at("alpha_test_discard"),
           "\tif (!(ALPHA_TEST_ACCESSOR > alphaTestRef)) {\n\t\tdiscard;\n\t}\n");
 EXPECT_EQ(snippets.at("iris_lightmap_matrix"),
           "const mat4 iris_lightmapTextureMatrix = mat4(vec4(0.00390625, 0.0, 0.0, 0.0), "
           "vec4(0.0, 0.00390625, 0.0, 0.0), vec4(0.0, 0.0, 0.00390625, 0.0), "
           "vec4(0.03125, 0.03125, 0.03125, 1.0));\n");
 EXPECT_EQ(snippets.at("mc_hand_depth"), "#define MC_HAND_DEPTH 0.125\n");
 EXPECT_EQ(snippets.at("chunk_fade_terrain_in"), "in float mc_chunkFade;\n");
 EXPECT_EQ(snippets.at("chunk_fade_other_const"), "const float mc_chunkFade = -1.0;\n");
 EXPECT_EQ(snippets.at("iris_fog_frag_coord_vertex_out"), "out float iris_FogFragCoord;\n");
 EXPECT_EQ(snippets.at("iris_fog_frag_coord_fragment_in"), "in float iris_FogFragCoord;\n");
 EXPECT_EQ(snippets.at("iris_fog_frag_coord_init_main"), "\tiris_FogFragCoord = 0.0f;\n");
 EXPECT_EQ(snippets.at("iris_front_color_global"), "vec4 iris_FrontColor;\n");
 EXPECT_EQ(snippets.at("gl_frag_depth_passthrough"), "\tgl_FragDepth = gl_FragCoord.z;\n");
 EXPECT_NE(snippets.at("default_composite.vsh").find("gl_Position = projectionMatrix * modelViewMatrix"),
           std::string::npos)
     << "default composite must keep the engine's own projection order";
  EXPECT_NE(snippets.at("default_raster.vsh").find("vaPosition + chunkOffset"), std::string::npos);
  EXPECT_EQ(snippets.at("colorwheel_macros_prefix"),
            "#define HAS_COLORWHEEL\n#define COLORWHEEL_VERSION \n");
  EXPECT_EQ(snippets.at("mc_texture_format_lab_pbr"), "#define MC_TEXTURE_FORMAT_LAB_PBR\n");
  EXPECT_EQ(snippets.at("mc_normal_specular_map"), "#define MC_NORMAL_MAP\n#define MC_SPECULAR_MAP\n");
}
} // namespace
} // namespace net::minecraft::test
