#pragma once

#include "seedfinder/api/SeedProbeTypes.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace seedfinder::engine {

constexpr int kBiomeCount = 13;

[[nodiscard]] std::string biomeNameFromId(std::uint8_t biome_id);
[[nodiscard]] std::string biomeDisplayName(std::uint8_t biome_id);
[[nodiscard]] bool biomeMatchesName(std::uint8_t biome_id, std::string_view name);
[[nodiscard]] std::string normalizeBiomeName(std::string_view name);

// Canonical snake_case names for config and rule editors.
[[nodiscard]] std::vector<std::string> allCanonicalBiomeNames();

// Spawn-biome picker order used by the in-game Seed Finder UI ("Any" is not included).
[[nodiscard]] std::vector<std::string> spawnPickerBiomeNames();

[[nodiscard]] int biomeChunkCount(const SeedProbeResult& result, std::string_view biome_name);

[[nodiscard]] bool metricNeedsBiomePick(std::string_view metric);
[[nodiscard]] bool metricNeedsBiomeGrid(std::string_view metric);

// Parses "{biome}_chunk_count" metrics. Returns empty view when not a chunk-count metric.
[[nodiscard]] std::string_view chunkCountBiomeFromMetric(std::string_view metric);

} // namespace seedfinder::engine
