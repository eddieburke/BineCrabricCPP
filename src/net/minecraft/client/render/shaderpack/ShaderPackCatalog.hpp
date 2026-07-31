#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack;
}
namespace net::minecraft::client::render::shaderpack {
// Filesystem helpers for discovering shader pack directories and zips.
namespace ShaderPackCatalog {
[[nodiscard]] std::string lower(std::string value);
[[nodiscard]] std::string readFile(const std::filesystem::path& path);
[[nodiscard]] std::vector<std::string> directoryResources(const std::filesystem::path& root);
[[nodiscard]] std::vector<std::string> zipResources(const resource::pack::ZippedTexturePack& zip);
[[nodiscard]] std::uint64_t packDirectoryStamp(const std::filesystem::path& root);
} // namespace ShaderPackCatalog
} // namespace net::minecraft::client::render::shaderpack
