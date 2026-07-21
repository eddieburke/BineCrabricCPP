#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::mod::runtime {
struct ZipEntry {
 std::string name;
 std::uint16_t compressionMethod = 0;
 std::uint32_t compressedSize = 0;
 std::uint32_t uncompressedSize = 0;
 std::uint32_t localHeaderOffset = 0;
};
bool buildZipIndex(const std::vector<std::uint8_t>& archive, std::vector<ZipEntry>& entries);
const ZipEntry* findZipEntry(const std::vector<ZipEntry>& entries, std::string_view path);
std::vector<std::uint8_t> readZipEntryData(const std::vector<std::uint8_t>& archive, const ZipEntry& entry);
ModPackage makeBrokenPackage(ModPackageSource source,
                             const std::filesystem::path& sourcePath,
                             std::string id,
                             std::string error);
bool parseManifestJson(const std::string& manifestTextRaw,
                       ModPackage& out,
                       const std::filesystem::path& sourcePath,
                       ModPackageSource source,
                       std::string errorPrefix);
void sortMods(std::vector<ModPackage>& mods);
} // namespace net::minecraft::mod::runtime
