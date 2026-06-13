#include "seedfinder/engine/SearchEngine.hpp"

#include "seedfinder/config/ScanConfigBuilder.hpp"
#include "seedfinder/probe/DecorateProbe.hpp"
#include "seedfinder/probe/ScanProbe.hpp"
#include "seedfinder/runtime/RuntimeInit.hpp"

#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>

namespace seedfinder::engine {
namespace {

constexpr std::uint64_t kBatchSize = 64;

// Splits a full PassPlan into the per-tier plans the cascade runs incrementally. Each
// tier's plan carries only the passes new to it, so re-running a wider probe doesn't
// redo cheaper work (ScanProbe gates biome/terrain sampling on the bits present).
struct TierPlans {
    PassPlan biome;     // biome sampling only
    PassPlan terrain;   // terrain-side passes (Terrain/Surface/Caves)
    PassPlan decorate;  // spawn + populate (needs a World)
    bool hasTerrain = false;
    bool hasDecorate = false;
    std::uint32_t afterBiome = 0;   // passes available after biome tier
    std::uint32_t afterTerrain = 0; // passes available after terrain tier
    std::uint32_t all = 0;
};

TierPlans splitPlan(const PassPlan& full)
{
    TierPlans t;
    t.all = full.passes;

    t.biome = full;
    t.biome.passes = pass::Biome;
    t.biome.lastDecorStep = 0;
    t.biome.needSpawn = false;
    t.afterBiome = pass::Biome;

    const std::uint32_t terrainBits = full.passes & pass::TerrainSide & ~pass::Biome;
    t.hasTerrain = terrainBits != 0;
    t.terrain = full;
    t.terrain.passes = terrainBits;
    t.terrain.lastDecorStep = 0;
    t.terrain.needSpawn = false;
    t.afterTerrain = pass::Biome | terrainBits;

    const std::uint32_t populateBits = full.passes & pass::Populate;
    t.hasDecorate = populateBits != 0 || full.needSpawn;
    t.decorate = full;
    t.decorate.passes = populateBits;

    return t;
}

// One unit of cascade work for a single seed. Returns true if the seed is a hit (all hard
// constraints met) and fills `hit`. Reuses the thread-local probes and result buffer.
bool evaluateSeed(
    std::uint64_t seed,
    const config::ScanConfig& cfg,
    const TierPlans& tiers,
    ScanProbe& scanProbe,
    DecorateProbe& decorateProbe,
    ProbeResult& result,
    SearchHit& hit)
{
    result.reset();
    const int ox = cfg.scan_origin_chunk_x;
    const int oz = cfg.scan_origin_chunk_z;

    // Tier 1: biomes.
    scanProbe.probe(seed, ox, oz, tiers.biome, result);
    graph::GraphVerdict v = cfg.graph.evaluate(result, result.passesRun);
    if (!v.hardPass) {
        return false;
    }

    // Tier 2: terrain side.
    if (tiers.hasTerrain) {
        scanProbe.probe(seed, ox, oz, tiers.terrain, result);
        v = cfg.graph.evaluate(result, result.passesRun);
        if (!v.hardPass) {
            return false;
        }
    }

    // Tier 3: spawn + populate (full World).
    if (tiers.hasDecorate) {
        decorateProbe.probe(seed, ox, oz, tiers.decorate, result);
    }

    // Final, exact verdict over every available pass.
    v = cfg.graph.evaluate(result, tiers.all);
    if (!v.hardPass) {
        return false;
    }

    hit.seed = seed;
    hit.score = v.score;
    hit.all_hard = true;
    return true;
}

void worker(
    const config::ScanConfig& cfg,
    const TierPlans& tiers,
    std::atomic<std::uint64_t>& cursor,
    std::uint64_t end,
    std::atomic<bool>* cancel_flag,
    std::atomic<std::uint64_t>* progress_counter,
    std::atomic<std::uint64_t>& checked,
    std::mutex& hit_mutex,
    std::vector<SearchHit>& hits,
    const HitCallback& callback)
{
    ScanProbe scanProbe;
    DecorateProbe decorateProbe;
    ProbeResult result;
    SearchHit hit;
    std::uint64_t localChecked = 0;

    for (;;) {
        if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
            break;
        }
        const std::uint64_t batchStart = cursor.fetch_add(kBatchSize, std::memory_order_relaxed);
        if (batchStart > end) {
            break;
        }
        const std::uint64_t batchEnd = std::min(end, batchStart + kBatchSize - 1);

        for (std::uint64_t seed = batchStart; seed <= batchEnd; ++seed) {
            if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
                break;
            }
            ++localChecked;
            if (progress_counter != nullptr) {
                progress_counter->fetch_add(1, std::memory_order_relaxed);
            }
            if (evaluateSeed(seed, cfg, tiers, scanProbe, decorateProbe, result, hit)) {
                hit.probe = result; // deep copy the surviving result into the hit
                std::lock_guard<std::mutex> lock(hit_mutex);
                hits.push_back(hit);
                if (callback) {
                    callback(hits.back());
                }
            }
        }
    }

    checked.fetch_add(localChecked, std::memory_order_relaxed);
}

} // namespace

SearchEngine::SearchEngine(config::SearchConfig cfg)
    : legacy_(std::move(cfg))
{
    cfg_ = config::toScanConfig(legacy_);
    checkpoint_.config_hash = hashConfig(legacy_);
    if (!legacy_.checkpoint_path.empty()) {
        loadCheckpoint(legacy_.checkpoint_path, checkpoint_);
    }
}

void SearchEngine::maintainTopHits(std::vector<SearchHit>& top, SearchHit&& hit)
{
    top.push_back(std::move(hit));
    std::sort(top.begin(), top.end(), [](const SearchHit& a, const SearchHit& b) {
        return a.score > b.score;
    });
    while (static_cast<int>(top.size()) > cfg_.top_k) {
        top.pop_back();
    }
}

SearchSummary SearchEngine::run()
{
    seedfinder::runtime::initialize();

    SearchSummary summary;
    const PassPlan full = cfg_.plan();
    const TierPlans tiers = splitPlan(full);

    std::uint64_t start = cfg_.seed_start;
    if (checkpoint_.cursor_seed > start) {
        start = checkpoint_.cursor_seed;
    }
    const std::uint64_t end = cfg_.seed_end;

    const unsigned hw = std::thread::hardware_concurrency();
    int threadCount = cfg_.threads > 0 ? cfg_.threads : static_cast<int>(hw == 0 ? 1u : hw);
    threadCount = std::max(1, threadCount);

    std::atomic<std::uint64_t> cursor {start};
    std::atomic<std::uint64_t> checked {0};
    std::mutex hitMutex;
    std::vector<SearchHit> rawHits;

    if (start <= end) {
        if (threadCount == 1) {
            worker(cfg_, tiers, cursor, end, cancel_flag_, progress_counter_,
                checked, hitMutex, rawHits, hit_callback_);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(static_cast<std::size_t>(threadCount));
            for (int t = 0; t < threadCount; ++t) {
                threads.emplace_back(worker, std::cref(cfg_), std::cref(tiers),
                    std::ref(cursor), end, cancel_flag_, progress_counter_,
                    std::ref(checked), std::ref(hitMutex), std::ref(rawHits),
                    std::cref(hit_callback_));
            }
            for (std::thread& th : threads) {
                th.join();
            }
        }
    }

    summary.seeds_checked = checked.load();
    summary.hits_found = rawHits.size();
    for (SearchHit& hit : rawHits) {
        maintainTopHits(summary.top_hits, std::move(hit));
    }

    const bool cancelled = cancel_flag_ != nullptr && cancel_flag_->load();
    if (!cancelled) {
        // Clean finish: advance the cursor past the whole range so a resume starts fresh.
        checkpoint_.cursor_seed = end + 1;
        checkpoint_.top_hits.clear();
        for (const SearchHit& hit : summary.top_hits) {
            checkpoint_.top_hits.push_back({hit.seed, hit.score});
        }
        if (!legacy_.checkpoint_path.empty()) {
            saveCheckpoint(legacy_.checkpoint_path, checkpoint_);
        }
    }

    return summary;
}

} // namespace seedfinder::engine
