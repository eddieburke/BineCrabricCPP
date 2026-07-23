#pragma once
#include <filesystem>
#include <string_view>
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
namespace net::minecraft::client::resource {
// Resource files are runtime data. The executable must not depend on a source-tree
// path or on shader source embedded in the binary. Always the persistent per-user game
// directory (%APPDATA%/.minecraft/resources on Windows); build-omega.ps1 mirrors
// native/resources/ there after each build (see Sync-Resources).
[[nodiscard]] inline std::filesystem::path resourceRoot() {
 return net::minecraft::client::util::MinecraftDirectories::getRunDirectory() / "resources";
}
[[nodiscard]] inline std::filesystem::path resolveResource(std::string_view relativePath) {
 return resourceRoot() / std::filesystem::path(relativePath).lexically_normal();
}
} // namespace net::minecraft::client::resource
