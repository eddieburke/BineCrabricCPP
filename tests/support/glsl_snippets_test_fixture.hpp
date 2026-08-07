#pragma once
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"

// Installs the engine GLSL snippet set into GlslSnippets so transform tests
// are hermetic: no %APPDATA%/.minecraft/resources, no texture packs, no GL.
// The text is read from the repo's own src/.../shaders/glsl/ source directory
// (MINECRAFT_TEST_SOURCE_DIR is baked into the test target by CMake), which is
// the same source the build embeds into the executable, so the fixture can
// never drift from the shipped snippets. The pure map-injection path is
// exercised directly by tests/glsl_snippets_test.cpp with hand-built maps.
namespace net::minecraft::test {
namespace {
const std::vector<std::string>& glslSnippetNames() {
 static const std::vector<std::string> names = {
     "default_composite.vsh",
     "default_raster.vsh",
     "iris_lightmap_matrix",
     "alpha_test_discard",
     "compat_alpha_check",
     "iris_fog_frag_coord_vertex_out",
     "iris_fog_frag_coord_init_main",
     "iris_fog_frag_coord_fragment_in",
     "iris_front_color_global",
      "chunk_fade_terrain_in",
      "chunk_fade_other_const",
      "gl_frag_depth_passthrough",
      "colorwheel_macros_prefix",
     "colorwheel_stage_defines",
     "colorwheel_vertex_bridge",
     "colorwheel_vertex_main",
     "colorwheel_fragment_bridge",
     "colorwheel_fragment_discard_cutout",
     "colorwheel_fragment_discard_translucent",
     "colorwheel_geometry_bridge",
     "colorspace_preamble",
     "colorspace_vertex",
     "colorspace_enum_defines",
     "colorspace_fragment"};
 return names;
}
std::string readSnippetFile(const std::string& name) {
 std::ifstream input(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "src" /
                         "net" / "minecraft" / "client" / "render" / "shaders" / "glsl" /
                         (name + ".glsl"),
                     std::ios::binary);
 if(!input) {
  return {};
 }
 return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
} // namespace
inline const std::unordered_map<std::string, std::string>& testGlslSnippets() {
 static const std::unordered_map<std::string, std::string> map = [] {
  std::unordered_map<std::string, std::string> snippets;
  for(const std::string& name : glslSnippetNames()) {
   snippets[name] = readSnippetFile(name);
  }
  return snippets;
 }();
 return map;
}
inline void installTestGlslSnippets() {
 net::minecraft::client::render::GlslSnippets::setSourceMapForTesting(testGlslSnippets());
}
} // namespace net::minecraft::test
