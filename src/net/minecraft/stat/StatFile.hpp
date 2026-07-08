#pragma once
#include <filesystem>
#include <unordered_map>

namespace net::minecraft::stat {
// Native stats file: magic "MNST", version 1, then (statId, value) pairs.
// No JSON, no checksums, no background threads.
[[nodiscard]] bool readStatFile(const std::filesystem::path& path, std::unordered_map<int, int>& out);
[[nodiscard]] bool writeStatFile(const std::filesystem::path& path, const std::unordered_map<int, int>& stats);
}  // namespace net::minecraft::stat
