#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace net::minecraft::client::render {
// Runtime loader for the engine's own GLSL snippets (resources/glsl/*.glsl).
// The engine must not embed shader text in the binary; snippets resolve through
// the same chain as game assets: selected texture pack first, then the game
// resources directory (%APPDATA%/.minecraft/resources, mirrored from
// resources/ by build-omega.ps1 Sync-Resources; see ResourceRoot.hpp).
// A missing snippet yields an empty string (logged once); callers degrade the
// same way as if the injection never ran (see docs/agent-notes/glsl.md).
class GlslSnippets {
 public:
  // Cached, thread-safe. Returns by value so callers never hold a reference
  // into the growing cache (unordered_map rehash would dangle it).
  [[nodiscard]] static std::string get(std::string_view name);

  // Test hook: replaces the file-backed source map and clears the cache.
  // An empty map restores disk-backed loading (the only reset needed; the
  // missing-snippet warning set is a dedup guard, not test state).
  static void setSourceMapForTesting(std::unordered_map<std::string, std::string> map);
};
} // namespace net::minecraft::client::render
