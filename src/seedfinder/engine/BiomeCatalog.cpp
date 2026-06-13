#include "seedfinder/engine/BiomeCatalog.hpp"

#include <algorithm>
#include <cctype>

namespace seedfinder::engine {
namespace {

struct BiomeEntry {
    const char* canonical;
    const char* display;
};

// Matches net::minecraft::BiomeId wire order used by seedfinder_probe.
constexpr BiomeEntry kBiomes[kBiomeCount] = {
    {"rainforest", "Rainforest"},
    {"swampland", "Swampland"},
    {"seasonal_forest", "Seasonal Forest"},
    {"forest", "Forest"},
    {"savanna", "Savanna"},
    {"shrubland", "Shrubland"},
    {"taiga", "Taiga"},
    {"desert", "Desert"},
    {"plains", "Plains"},
    {"ice_desert", "Ice Desert"},
    {"tundra", "Tundra"},
    {"hell", "Hell"},
    {"sky", "Sky"},
};

constexpr const char* kSpawnPickerOrder[] = {
    "plains",
    "forest",
    "desert",
    "taiga",
    "swampland",
    "rainforest",
    "savanna",
    "shrubland",
    "seasonal_forest",
    "ice_desert",
    "tundra",
    "hell",
    "sky",
};

constexpr std::string_view kChunkCountSuffix = "_chunk_count";

std::string normalizeBiomeNameLocal(std::string_view name)
{
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (c == ' ') {
            out.push_back('_');
        } else {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

bool namesEqualNormalized(std::string_view left, std::string_view right)
{
    return normalizeBiomeNameLocal(left) == normalizeBiomeNameLocal(right);
}

bool pickerNameKnown(std::string_view name)
{
    for (const char* picker : kSpawnPickerOrder) {
        if (namesEqualNormalized(name, picker)) {
            return true;
        }
    }
    return false;
}

} // namespace

std::string normalizeBiomeName(std::string_view name)
{
    return normalizeBiomeNameLocal(name);
}

std::string biomeNameFromId(std::uint8_t biome_id)
{
    if (biome_id >= kBiomeCount) {
        return "unknown";
    }
    return kBiomes[biome_id].canonical;
}

std::string biomeDisplayName(std::uint8_t biome_id)
{
    if (biome_id >= kBiomeCount) {
        return "Unknown";
    }
    return kBiomes[biome_id].display;
}

std::uint8_t biomeIdFromName(std::string_view name)
{
    const std::string normalized = normalizeBiomeName(name);
    for (int i = 0; i < kBiomeCount; ++i) {
        if (normalized == kBiomes[i].canonical
            || normalized == normalizeBiomeName(kBiomes[i].display)) {
            return static_cast<std::uint8_t>(i);
        }
    }
    return 0;
}

bool biomeMatchesName(std::uint8_t biome_id, std::string_view name)
{
    if (biome_id >= kBiomeCount) {
        return false;
    }
    const std::string normalized = normalizeBiomeName(name);
    if (normalized == kBiomes[biome_id].canonical) {
        return true;
    }
    return normalized == normalizeBiomeName(kBiomes[biome_id].display);
}

std::vector<std::string> allCanonicalBiomeNames()
{
    std::vector<std::string> names;
    names.reserve(kBiomeCount);
    for (const BiomeEntry& entry : kBiomes) {
        names.emplace_back(entry.canonical);
    }
    return names;
}

std::vector<std::string> spawnPickerBiomeNames()
{
    std::vector<std::string> names;
    names.reserve(sizeof(kSpawnPickerOrder) / sizeof(kSpawnPickerOrder[0]));
    for (const char* picker : kSpawnPickerOrder) {
        names.emplace_back(picker);
    }
    return names;
}

std::string_view chunkCountBiomeFromMetric(std::string_view metric)
{
    if (metric.size() <= kChunkCountSuffix.size()) {
        return {};
    }
    if (metric.substr(metric.size() - kChunkCountSuffix.size()) != kChunkCountSuffix) {
        return {};
    }
    const std::string_view biome = metric.substr(0, metric.size() - kChunkCountSuffix.size());
    if (biome.empty()) {
        return {};
    }
    for (const BiomeEntry& entry : kBiomes) {
        if (namesEqualNormalized(biome, entry.canonical)) {
            return entry.canonical;
        }
    }
    if (pickerNameKnown(biome)) {
        return biome;
    }
    return {};
}

int biomeChunkCount(const SeedProbeResult& result, std::string_view biome_name)
{
    if (result.biome_grid != nullptr && result.biome_grid_len > 0) {
        int count = 0;
        for (std::uint32_t i = 0; i < result.biome_grid_len; ++i) {
            if (biomeMatchesName(result.biome_grid[i].biome_id, biome_name)) {
                ++count;
            }
        }
        return count;
    }
    if (namesEqualNormalized(biome_name, "desert")) {
        return static_cast<int>(result.desert_chunk_count);
    }
    return 0;
}

bool metricNeedsBiomePick(std::string_view metric)
{
    if (metric == "biome_coverage_percent" || metric == "max_contiguous_biome_radius"
        || metric == "biome_blob_compactness_percent" || metric == "biome_chunk_count") {
        return true;
    }
    return !chunkCountBiomeFromMetric(metric).empty();
}

bool metricNeedsBiomeGrid(std::string_view metric)
{
    if (metric == "biome_coverage_percent" || metric == "max_contiguous_biome_radius"
        || metric == "biome_blob_compactness_percent" || metric == "biome_chunk_count") {
        return true;
    }
    return !chunkCountBiomeFromMetric(metric).empty();
}

} // namespace seedfinder::engine
