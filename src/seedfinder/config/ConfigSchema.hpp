#pragma once

#include "seedfinder/api/SeedProbeTypes.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace seedfinder::config {

constexpr int kParamCount = 84;
struct RuleNode;

enum class ParamWireStatus : std::uint8_t {
    Wired,
    SchemaOnly,
    Stub,
    NotApplicable,
};

enum class ParamId : int {
    Seed = 1,
    SeedRangeStart = 2,
    SeedRangeEnd = 3,
    Dimension = 4,
    ScanOriginX = 5,
    ScanOriginZ = 6,
    ScanRadiusChunks = 7,
    ProbeDepth = 8,
    SpawnBiome = 9,
    SpawnBiomeIn = 10,
    SpawnX = 11,
    SpawnZ = 12,
    SpawnSurfaceBlock = 13,
    SpawnY = 14,
    SpawnDistanceChunks = 15,
    SpawnChunkX = 16,
    BiomeAtX = 17,
    BiomeAtZ = 18,
    BiomeAtRequired = 19,
    UniqueBiomeMin = 20,
    UniqueBiomeMax = 21,
    TemperatureMin = 22,
    TemperatureMax = 23,
    DownfallMin = 24,
    DownfallMax = 25,
    RequireSnow = 26,
    RequireRain = 27,
    ForbiddenBiomes = 28,
    RequiredBiomes = 29,
    AvgSurfaceYMin = 30,
    AvgSurfaceYMax = 31,
    FlatnessMax = 32,
    UnderwaterPercentMax = 33,
    GravelPatchMin = 34,
    SandPatchMin = 35,
    CaveProxyMin = 36,
    CoalOreMin = 37,
    IronOreMin = 38,
    GoldOreMin = 39,
    DiamondOreMin = 40,
    RedstoneOreMin = 41,
    LapisOreMin = 42,
    ClayBlobMin = 43,
    OreClusterMin = 44,
    OreClusterMax = 45,
    OreDepthMin = 46,
    OreDepthMax = 47,
    OreNearSpawnMin = 48,
    OreNearSpawnMax = 49,
    OreForbidden = 50,
    DungeonMin = 51,
    DungeonNearSpawnMin = 52,
    DungeonMob = 53,
    DungeonDepthMin = 54,
    DungeonDepthMax = 55,
    DungeonDistanceMax = 56,
    DungeonForbidden = 57,
    DungeonRequired = 58,
    WaterLakeMin = 59,
    LavaLakeMin = 60,
    WaterSpringMin = 61,
    LavaSpringMin = 62,
    FluidNearSpawnMin = 63,
    FluidForbidden = 64,
    FluidRequired = 65,
    TreesMin = 66,
    PumpkinPatchMin = 67,
    SugarCaneMin = 68,
    BrownMushroomMin = 69,
    RedMushroomMin = 70,
    CactusMin = 71,
    SnowBlocksMin = 72,
    FloraNearSpawnMin = 73,
    FloraForbidden = 74,
    FloraRequired = 75,
    NetherFortress = 76,
    NetherGlowstoneMin = 77,
    NetherLavaSea = 78,
    NetherBiome = 79,
    NetherSpawn = 80,
    AllHardConstraintsMet = 81,
    PartialMatchScore = 82,
    SearchTierPassed = 83,
    Notes = 84,
};

struct ParamDescriptor {
    ParamId id;
    const char* json_key;
    ParamWireStatus wire_status;
};

constexpr ParamDescriptor kParamRegistry[kParamCount] = {
    {ParamId::Seed, "seed", ParamWireStatus::Wired},
    {ParamId::SeedRangeStart, "seed_range_start", ParamWireStatus::Wired},
    {ParamId::SeedRangeEnd, "seed_range_end", ParamWireStatus::Wired},
    {ParamId::Dimension, "dimension", ParamWireStatus::Wired},
    {ParamId::ScanOriginX, "scan_origin_x", ParamWireStatus::Wired},
    {ParamId::ScanOriginZ, "scan_origin_z", ParamWireStatus::Wired},
    {ParamId::ScanRadiusChunks, "scan_radius_chunks", ParamWireStatus::Wired},
    {ParamId::ProbeDepth, "probe_depth", ParamWireStatus::Wired},
    {ParamId::SpawnBiome, "spawn_biome", ParamWireStatus::Wired},
    {ParamId::SpawnBiomeIn, "spawn_biome_in", ParamWireStatus::Wired},
    {ParamId::SpawnX, "spawn_x", ParamWireStatus::Wired},
    {ParamId::SpawnZ, "spawn_z", ParamWireStatus::Wired},
    {ParamId::SpawnSurfaceBlock, "spawn_surface_block", ParamWireStatus::Wired},
    {ParamId::SpawnY, "spawn_y", ParamWireStatus::Wired},
    {ParamId::SpawnDistanceChunks, "spawn_distance_chunks", ParamWireStatus::Wired},
    {ParamId::SpawnChunkX, "spawn_chunk_x", ParamWireStatus::Wired},
    {ParamId::BiomeAtX, "biome_at_x", ParamWireStatus::Wired},
    {ParamId::BiomeAtZ, "biome_at_z", ParamWireStatus::Wired},
    {ParamId::BiomeAtRequired, "biome_at_required", ParamWireStatus::Wired},
    {ParamId::UniqueBiomeMin, "unique_biome_min", ParamWireStatus::Wired},
    {ParamId::UniqueBiomeMax, "unique_biome_max", ParamWireStatus::Wired},
    {ParamId::TemperatureMin, "temperature_min", ParamWireStatus::Wired},
    {ParamId::TemperatureMax, "temperature_max", ParamWireStatus::Wired},
    {ParamId::DownfallMin, "downfall_min", ParamWireStatus::Wired},
    {ParamId::DownfallMax, "downfall_max", ParamWireStatus::Wired},
    {ParamId::RequireSnow, "require_snow", ParamWireStatus::Wired},
    {ParamId::RequireRain, "require_rain", ParamWireStatus::Wired},
    {ParamId::ForbiddenBiomes, "forbidden_biomes", ParamWireStatus::Wired},
    {ParamId::RequiredBiomes, "required_biomes", ParamWireStatus::Wired},
    {ParamId::AvgSurfaceYMin, "avg_surface_y_min", ParamWireStatus::Wired},
    {ParamId::AvgSurfaceYMax, "avg_surface_y_max", ParamWireStatus::Wired},
    {ParamId::FlatnessMax, "flatness_max", ParamWireStatus::Wired},
    {ParamId::UnderwaterPercentMax, "underwater_percent_max", ParamWireStatus::Wired},
    {ParamId::GravelPatchMin, "gravel_patch_min", ParamWireStatus::SchemaOnly},
    {ParamId::SandPatchMin, "sand_patch_min", ParamWireStatus::SchemaOnly},
    {ParamId::CaveProxyMin, "cave_proxy_min", ParamWireStatus::Wired},
    {ParamId::CoalOreMin, "coal_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::IronOreMin, "iron_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::GoldOreMin, "gold_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::DiamondOreMin, "diamond_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::RedstoneOreMin, "redstone_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::LapisOreMin, "lapis_ore_min", ParamWireStatus::SchemaOnly},
    {ParamId::ClayBlobMin, "clay_blob_min", ParamWireStatus::SchemaOnly},
    {ParamId::OreClusterMin, "ore_cluster_min", ParamWireStatus::SchemaOnly},
    {ParamId::OreClusterMax, "ore_cluster_max", ParamWireStatus::SchemaOnly},
    {ParamId::OreDepthMin, "ore_depth_min", ParamWireStatus::SchemaOnly},
    {ParamId::OreDepthMax, "ore_depth_max", ParamWireStatus::SchemaOnly},
    {ParamId::OreNearSpawnMin, "ore_near_spawn_min", ParamWireStatus::SchemaOnly},
    {ParamId::OreNearSpawnMax, "ore_near_spawn_max", ParamWireStatus::SchemaOnly},
    {ParamId::OreForbidden, "ore_forbidden", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonMin, "dungeon_min", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonNearSpawnMin, "dungeon_near_spawn_min", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonMob, "dungeon_mob", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonDepthMin, "dungeon_depth_min", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonDepthMax, "dungeon_depth_max", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonDistanceMax, "dungeon_distance_max", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonForbidden, "dungeon_forbidden", ParamWireStatus::SchemaOnly},
    {ParamId::DungeonRequired, "dungeon_required", ParamWireStatus::SchemaOnly},
    {ParamId::WaterLakeMin, "water_lake_min", ParamWireStatus::SchemaOnly},
    {ParamId::LavaLakeMin, "lava_lake_min", ParamWireStatus::SchemaOnly},
    {ParamId::WaterSpringMin, "water_spring_min", ParamWireStatus::SchemaOnly},
    {ParamId::LavaSpringMin, "lava_spring_min", ParamWireStatus::SchemaOnly},
    {ParamId::FluidNearSpawnMin, "fluid_near_spawn_min", ParamWireStatus::SchemaOnly},
    {ParamId::FluidForbidden, "fluid_forbidden", ParamWireStatus::SchemaOnly},
    {ParamId::FluidRequired, "fluid_required", ParamWireStatus::SchemaOnly},
    {ParamId::TreesMin, "trees_min", ParamWireStatus::SchemaOnly},
    {ParamId::PumpkinPatchMin, "pumpkin_patch_min", ParamWireStatus::SchemaOnly},
    {ParamId::SugarCaneMin, "sugar_cane_min", ParamWireStatus::SchemaOnly},
    {ParamId::BrownMushroomMin, "brown_mushroom_min", ParamWireStatus::SchemaOnly},
    {ParamId::RedMushroomMin, "red_mushroom_min", ParamWireStatus::SchemaOnly},
    {ParamId::CactusMin, "cactus_min", ParamWireStatus::SchemaOnly},
    {ParamId::SnowBlocksMin, "snow_blocks_min", ParamWireStatus::SchemaOnly},
    {ParamId::FloraNearSpawnMin, "flora_near_spawn_min", ParamWireStatus::SchemaOnly},
    {ParamId::FloraForbidden, "flora_forbidden", ParamWireStatus::SchemaOnly},
    {ParamId::FloraRequired, "flora_required", ParamWireStatus::SchemaOnly},
    {ParamId::NetherFortress, "nether_fortress", ParamWireStatus::Stub},
    {ParamId::NetherGlowstoneMin, "nether_glowstone_min", ParamWireStatus::Stub},
    {ParamId::NetherLavaSea, "nether_lava_sea", ParamWireStatus::Stub},
    {ParamId::NetherBiome, "nether_biome", ParamWireStatus::Stub},
    {ParamId::NetherSpawn, "nether_spawn", ParamWireStatus::Stub},
    {ParamId::AllHardConstraintsMet, "all_hard_constraints_met", ParamWireStatus::Wired},
    {ParamId::PartialMatchScore, "partial_match_score", ParamWireStatus::Wired},
    {ParamId::SearchTierPassed, "search_tier_passed", ParamWireStatus::Wired},
    {ParamId::Notes, "notes", ParamWireStatus::Wired},
};

[[nodiscard]] inline const char* wireStatusLabel(ParamWireStatus status)
{
    switch (status) {
    case ParamWireStatus::Wired:
        return "Wired";
    case ParamWireStatus::SchemaOnly:
        return "Schema-only";
    case ParamWireStatus::Stub:
        return "Stub";
    default:
        return "N/A";
    }
}

enum class RuleNodeKind : std::uint8_t {
    BlockConstraint,
    BlockOnBlock,
    BiomeConstraint,
    MetricObjective,
};

enum class NbtValueKind : std::uint8_t {
    Compound,
    List,
    String,
    Number,
    Bool,
};

struct NbtValue {
    NbtValueKind kind = NbtValueKind::Compound;
    std::unordered_map<std::string, NbtValue> compound;
    std::vector<NbtValue> list;
    std::string string_value;
    double number_value = 0.0;
    bool bool_value = false;
};

struct RuleNode {
    RuleNodeKind kind = RuleNodeKind::BiomeConstraint;
    std::string metric;
    std::string op;
    std::string direction;
    std::vector<std::string> values;
    int block_id = -1;
    int block_below_id = -1;
    // BiomeConstraint: min_value = min coverage %, max_value = max coverage %,
    // value = min contiguous blob radius (chunks). BlockConstraint: block counts.
    // Use advanced objectives for biome_blob_compactness_percent ranking.
    double min_value = -1.0;
    double max_value = -1.0;
    double value = -1.0;
    float weight = 1.f;
};

struct SearchConfig {
    std::uint64_t seed_start = 0;
    std::uint64_t seed_end = 0;
    std::string dimension = "overworld";
    int scan_origin_x = 0;
    int scan_origin_z = 0;
    int scan_radius_chunks = 4;
    SeedProbeDepth probe_depth_max = kDepthTerrain;
    bool compute_spawn = false;

    std::string spawn_biome;
    std::vector<std::string> spawn_biome_in;
    int spawn_x = -1;
    int spawn_z = -1;
    std::string spawn_surface_block;
    int spawn_y_min = -1;
    int spawn_y_max = -1;
    int spawn_distance_chunks_max = -1;
    int spawn_chunk_x = -1;

    int biome_at_x = 0;
    int biome_at_z = 0;
    std::string biome_at_required;
    int unique_biome_min = -1;
    int unique_biome_max = -1;
    float temperature_min = -1.f;
    float temperature_max = -1.f;
    float downfall_min = -1.f;
    float downfall_max = -1.f;
    bool require_snow = false;
    bool require_rain = false;
    std::vector<std::string> forbidden_biomes;
    std::vector<std::string> required_biomes;
    float required_biome_min_coverage_percent = -1.f;
    float forbidden_biome_max_coverage_percent = -1.f;
    int required_biome_min_contiguous_radius_chunks = -1;
    float required_biome_min_compactness_percent = -1.f;

    float avg_surface_y_min = -1.f;
    float avg_surface_y_max = -1.f;
    float flatness_max = -1.f;
    float underwater_percent_max = -1.f;
    int cave_proxy_min = -1;

    int threads = 1;
    int top_k = 10;
    std::string checkpoint_path;
    std::string output_path;
    std::string search_mode = "brute";
    std::string advanced_rule_data_nbt;
    std::vector<RuleNode> advanced_rule_nodes;
    std::vector<std::string> advanced_rule_errors;

    std::unordered_map<std::string, std::string> raw_values;
    std::vector<std::string> schema_only_keys;
    std::vector<std::string> notes;
};

[[nodiscard]] const ParamDescriptor* findParamByKey(const std::string& key);
[[nodiscard]] bool parseAdvancedRuleDataNbt(
    const std::string& blob,
    std::vector<RuleNode>& out,
    std::vector<std::string>& errors);

// Serializes a rule-node list back into the advanced_rule_data_nbt blob format
// consumed by parseAdvancedRuleDataNbt(). Round-trips: parsing the result of
// serializeAdvancedRuleDataNbt(nodes) reproduces the same supported node set.
[[nodiscard]] std::string serializeAdvancedRuleDataNbt(const std::vector<RuleNode>& nodes);

} // namespace seedfinder::config
