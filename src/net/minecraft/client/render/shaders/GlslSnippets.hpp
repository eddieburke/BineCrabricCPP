#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace net::minecraft::client::render {
// Runtime loader for the engine's own GLSL snippets. The snippet text is
// embedded in the executable: CMake bakes src/.../shaders/glsl/*.glsl into a
// generated table (embeddedGlslSnippets) at configure time, so there is no
// resource-directory or texture-pack lookup at runtime.
// A missing snippet yields an empty string (logged once); callers degrade the
// same way as if the injection never ran.
class GlslSnippets {
 public:
  // Returns the embedded name -> source table for the engine GLSL snippets.
  // Defined in a build-generated TU (EmbeddedGlslSnippets.cpp) so the shader
  // text ships inside the executable rather than in a resources directory.
  [[nodiscard]] static const std::unordered_map<std::string, std::string>& embeddedGlslSnippets();

  // Cached, thread-safe. Returns by value so callers never hold a reference
  // into the growing cache (unordered_map rehash would dangle it).
  [[nodiscard]] static std::string get(std::string_view name);

  // Test hook: replaces the embedded source map and clears the cache.
  // An empty map restores the embedded table (the only reset needed; the
  // missing-snippet warning set is a dedup guard, not test state).
  static void setSourceMapForTesting(std::unordered_map<std::string, std::string> map);
};
} // namespace net::minecraft::client::render
