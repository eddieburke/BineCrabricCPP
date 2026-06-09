#pragma once

#include "seedfinder/engine/Pass.hpp"
#include "seedfinder/probe/ProbeResult.hpp"

#include <cstdint>

// Per-thread driver for the decorate (populate) tier. Unlike ScanProbe this needs a real
// World (populate features mutate the world across chunk borders), so it is the expensive
// opt-in tier — the engine only runs it for seeds that survive the cheaper terrain-side
// constraints. It truncates the populate chain to plan.lastDecorStep and tallies the
// decorate metrics into `out` (which already holds terrain-side data).
//
// Region note: the chunk cache only serves chunks within ~15 of one spawn-window centre,
// so the decorate scan is clamped to a 14-chunk radius around the scan origin.

namespace seedfinder {

class DecorateProbe {
public:
    void probe(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out);

    static constexpr int kMaxDecorateRadius = 14;
};

} // namespace seedfinder
