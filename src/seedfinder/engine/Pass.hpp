#pragma once

#include <cstdint>

// Generation-pass model for the seed finder.
//
// The scanner runs the game's faithful generator one stage at a time, gated by a
// resolved pass bitmask. Terrain-side passes (Biome/Terrain/Surface/Caves) run on a
// lean raw-Chunk grid with no World; the populate passes (Lakes..Snow) require a World
// and run as a truncated prefix of the decorate chain (see OverworldChunkGenerator::
// DecorStep). Metrics declare which passes they need; the planner unions them.

namespace seedfinder {

namespace pass {

constexpr std::uint32_t Biome    = 1u << 0;
constexpr std::uint32_t Terrain  = 1u << 1;
constexpr std::uint32_t Surface  = 1u << 2;
constexpr std::uint32_t Caves    = 1u << 3;
constexpr std::uint32_t Lakes    = 1u << 4;
constexpr std::uint32_t Dungeons = 1u << 5;
constexpr std::uint32_t Ores     = 1u << 6;
constexpr std::uint32_t Trees    = 1u << 7;
constexpr std::uint32_t Plants   = 1u << 8;
constexpr std::uint32_t Springs  = 1u << 9;
constexpr std::uint32_t Snow     = 1u << 10;

constexpr std::uint32_t Populate    = Lakes | Dungeons | Ores | Trees | Plants | Springs | Snow;
constexpr std::uint32_t TerrainSide = Biome | Terrain | Surface | Caves;

} // namespace pass

// Expands a requested pass mask to include its prerequisites, matching vanilla order:
//   any populate pass -> Terrain + Surface + Caves (vanilla carves before populate)
//   Caves   -> Terrain + Surface
//   Surface -> Terrain
//   Terrain -> Biome (terrain sampling reads the biome temperature map)
[[nodiscard]] constexpr std::uint32_t resolvePassDeps(std::uint32_t passes)
{
    if (passes & pass::Populate) {
        passes |= pass::Terrain | pass::Surface | pass::Caves;
    }
    if (passes & pass::Caves) {
        passes |= pass::Terrain | pass::Surface;
    }
    if (passes & pass::Surface) {
        passes |= pass::Terrain;
    }
    if (passes & pass::Terrain) {
        passes |= pass::Biome;
    }
    return passes;
}

// Highest decorate step needed for a resolved mask, as an OverworldChunkGenerator::
// DecorStep ordinal (Lakes=1 .. Snow=7); 0 means no populate. The decorate chain runs
// Lakes..thisStep and stops — skipped tail groups are bit-identical no-ops for anything
// measured, since they come later in the shared RNG stream.
[[nodiscard]] constexpr int lastPopulateStep(std::uint32_t passes)
{
    if (passes & pass::Snow) {
        return 7;
    }
    if (passes & pass::Springs) {
        return 6;
    }
    if (passes & pass::Plants) {
        return 5;
    }
    if (passes & pass::Trees) {
        return 4;
    }
    if (passes & pass::Ores) {
        return 3;
    }
    if (passes & pass::Dungeons) {
        return 2;
    }
    if (passes & pass::Lakes) {
        return 1;
    }
    return 0;
}

// A resolved plan for one scan: which passes to run, how far to take the decorate
// chain, the region size, and whether a World is required (any populate pass).
struct PassPlan {
    std::uint32_t passes = 0;
    int lastDecorStep = 0;   // 0 = no decorate; else OverworldChunkGenerator::DecorStep
    int radiusChunks = 4;
    int caveMargin = 8;      // extra ring carved so cross-chunk cave systems are faithful
    bool needSpawn = false;

    [[nodiscard]] bool needsWorld() const noexcept { return (passes & pass::Populate) != 0; }
    [[nodiscard]] bool needsCaves() const noexcept { return (passes & pass::Caves) != 0; }
    [[nodiscard]] bool has(std::uint32_t bit) const noexcept { return (passes & bit) != 0; }
};

[[nodiscard]] inline PassPlan makePassPlan(std::uint32_t requestedPasses, int radiusChunks, bool needSpawn)
{
    PassPlan plan;
    plan.passes = resolvePassDeps(requestedPasses);
    plan.lastDecorStep = lastPopulateStep(plan.passes);
    plan.radiusChunks = radiusChunks;
    plan.caveMargin = plan.needsCaves() ? 8 : 0;
    plan.needSpawn = needSpawn;
    return plan;
}

} // namespace seedfinder
