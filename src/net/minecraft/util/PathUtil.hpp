#pragma once
#include <string>
#include <string_view>
namespace net::minecraft::util {
// Strips leading '/' or '\' — the canonical resource-path form TextureManager's
// path maps and TextureRegistry's registry keys both key on. Lives in util (not
// client) because TextureRegistry is part of minecraft_core, which the dedicated
// server links without the client library.
[[nodiscard]] inline std::string_view normalizePath(std::string_view path) {
 const std::size_t first = path.find_first_not_of("/\\");
 if(first == std::string_view::npos) return {};
 return path.substr(first);
}
} // namespace net::minecraft::util
