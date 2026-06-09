// seed_probe_golden_test.cpp — golden-vector skeleton for seedfinder_probe adapter.
// Compile fixer captures fixtures into tests/fixtures/seed_probe_golden.json.

#include "seedfinder/api/SeedProbeApi.h"
#include "seedfinder/api/SeedProbeTypes.hpp"
#include "seedfinder/runtime/RuntimeInit.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace {

void initTerrainRequest(SeedProbeRequest& req, std::uint64_t seed)
{
    seedfinder::initDefaultRequest(req);
    req.seed = seed;
    req.dimension = SEEDFINDER_DIM_OVERWORLD;
    req.depth = SEEDFINDER_DEPTH_TERRAIN;
    req.compute_spawn = 1;
    req.region.radius_chunks = 2;
    req.region.cave_margin_chunks = 8;
}

void initBiomeOnlyRequest(SeedProbeRequest& req, std::uint64_t seed)
{
    seedfinder::initDefaultRequest(req);
    req.seed = seed;
    req.dimension = SEEDFINDER_DIM_OVERWORLD;
    req.depth = SEEDFINDER_DEPTH_BIOME_ONLY;
    req.compute_spawn = 0;
}

bool probeOk(std::uint64_t seed, seedfinder::SeedProbeDepth depth, SeedProbeResult& out)
{
    SeedProbeRequest req {};
    if (depth == SEEDFINDER_DEPTH_BIOME_ONLY) {
        initBiomeOnlyRequest(req, seed);
    } else {
        initTerrainRequest(req, seed);
    }

    SeedProbeScratch* scratch = seedfinder_scratch_create();
    assert(scratch != nullptr);

    char error[256] = {};
    seedfinder::initDefaultResult(out);
    const int rc = seedfinder_probe(&req, scratch, &out, error, sizeof(error));
    seedfinder_scratch_destroy(scratch);
    if (rc != 0) {
        std::fprintf(stderr, "probe failed seed=%llu rc=%d err=%s\n",
            static_cast<unsigned long long>(seed), rc, error);
        return false;
    }
    return true;
}

void test_g_probe_12345()
{
    SeedProbeResult out {};
    assert(probeOk(12345, SEEDFINDER_DEPTH_TERRAIN, out));
    assert(out.spawn.valid != 0);
    assert(out.spawn.on_sand_beach != 0);
    assert(out.depth_reached >= SEEDFINDER_DEPTH_TERRAIN);
    seedfinder_result_clear(&out);
}

void test_g_probe_zero()
{
    SeedProbeResult out {};
    assert(probeOk(0, SEEDFINDER_DEPTH_BIOME_ONLY, out));
    assert(out.unique_biome_count >= 1);
    seedfinder_result_clear(&out);
}

void test_g_probe_max()
{
    SeedProbeResult out {};
    const std::uint64_t seed = 9223372036854775807ULL;
    assert(probeOk(seed, SEEDFINDER_DEPTH_BIOME_ONLY, out));
    assert(out.seed == seed);
    seedfinder_result_clear(&out);
}

void test_g_probe_negative()
{
    SeedProbeResult out {};
    const std::uint64_t seed = static_cast<std::uint64_t>(-1LL);
    assert(probeOk(seed, SEEDFINDER_DEPTH_TERRAIN, out));
    assert(out.seed == seed);
    seedfinder_result_clear(&out);
}

void test_g_probe_text_hash()
{
    // "hello" -> javaStringHashCode = 99162322
    SeedProbeResult out {};
    assert(probeOk(99162322ULL, SEEDFINDER_DEPTH_BIOME_ONLY, out));
    seedfinder_result_clear(&out);
}

} // namespace

int main()
{
    seedfinder::runtime::initialize();
    test_g_probe_12345();
    test_g_probe_zero();
    test_g_probe_max();
    test_g_probe_negative();
    test_g_probe_text_hash();
    std::printf("seed_probe_golden_test: all skeleton checks passed\n");
    return 0;
}
