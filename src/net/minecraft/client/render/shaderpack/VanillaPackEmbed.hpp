#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render {
// The built-in vanilla shader pack baked into the executable. The map is generated
// from shaders/vanilla/** at configure time (EmbeddedVanillaPack.cpp), keyed by
// pack-relative resource paths ("shaders/gbuffers_terrain.vsh") exactly as
// PackLoader addresses pack files. The runtime falls back to these when no
// shaders/vanilla directory exists next to the game, so the pack ships in the
// binary itself.
class VanillaPackEmbed {
 public:
  [[nodiscard]] static const std::unordered_map<std::string, std::string>& embeddedResources();

  [[nodiscard]] static std::vector<std::string> resources();

  [[nodiscard]] static std::string get(std::string_view path);

  [[nodiscard]] static bool has(std::string_view path);
};
} // namespace net::minecraft::client::render
