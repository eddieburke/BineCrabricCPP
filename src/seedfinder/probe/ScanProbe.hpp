#pragma once

#include "seedfinder/engine/Pass.hpp"
#include "seedfinder/probe/ProbeResult.hpp"

#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace seedfinder {

// Per-thread scratch + driver. Probes one seed at a chunk-coord origin under a PassPlan,
// reusing the game's faithful generator stages with no World (terrain-side passes only).
// The decorate tier (populate features) attaches in Phase 3 via the World path.
//
// A single scratch Chunk serves every cell: genTerrain/genSurface/carve write blocks by
// (chunkX,chunkZ) argument and never read Chunk::x/z, so the buffer's own position is
// irrelevant and the whole grid is generated through one reused chunk.
class ScanProbe {
public:
    ScanProbe() = default;

    void probe(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out);

private:
    void sampleBiomeGrid(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out);
    void runTerrainSide(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out);

    net::minecraft::OverworldChunkGenerator& ensureGenerator(std::uint64_t seed);

    net::minecraft::BiomeSource biomeSource_ {0};
    std::optional<net::minecraft::OverworldChunkGenerator> generator_;
    std::uint64_t generatorSeed_ = ~0ull;
    bool generatorValid_ = false;

    net::minecraft::Chunk scratch_ {nullptr, 0, 0};
    std::vector<net::minecraft::Biome*> biomeArea_;
};

} // namespace seedfinder
