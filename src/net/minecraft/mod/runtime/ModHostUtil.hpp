#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::mod::runtime {
inline constexpr std::uintmax_t kMaxModArchiveBytes = 256U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModEntryBytes = 64U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModExtractedBytes = 512U * 1024U * 1024U;
inline constexpr std::uint16_t kMaxModArchiveEntries = 4096;
std::string trimCopy(std::string value);
std::string toLowerCopy(std::string value);
std::string snakeToCamel(std::string_view snake);
std::string normalizeRelativePath(std::string_view value);
bool isSafeRelativePath(std::string_view value);
bool isSafeModId(std::string_view value);
bool isDirectoryZipPath(std::string_view value);
std::string sanitizeName(std::string_view value);
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path);
std::string readFileText(const std::filesystem::path& path);
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes);
bool writeFileText(const std::filesystem::path& path, const std::string& text);
}  // namespace net::minecraft::mod::runtime
