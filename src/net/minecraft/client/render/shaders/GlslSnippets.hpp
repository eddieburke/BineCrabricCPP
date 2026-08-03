#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace net::minecraft::client::render {
class GlslSnippets {
 public:
  [[nodiscard]] static const std::unordered_map<std::string, std::string>& embeddedGlslSnippets();

  [[nodiscard]] static std::string get(std::string_view name);

  static void setSourceMapForTesting(std::unordered_map<std::string, std::string> map);
};
} // namespace net::minecraft::client::render
