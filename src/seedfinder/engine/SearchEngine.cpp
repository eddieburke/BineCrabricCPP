#include "seedfinder/engine/SearchEngine.hpp"

#include "seedfinder/api/SeedProbeApi.h"
#include "seedfinder/engine/TierPlanner.hpp"
#include "seedfinder/runtime/RuntimeInit.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace seedfinder::engine {
namespace {

struct ThreadContext {
    SeedProbeScratch* scratch = nullptr;
};

void workerRange(
    const config::SearchConfig& cfg,
    const TierPlan& plan,
    std::uint64_t start,
    std::uint64_t end,
    std::atomic<bool>* cancel_flag,
    std::atomic<std::uint64_t>* progress_counter,
    std::atomic<std::uint64_t>& checked,
    std::mutex& hit_mutex,
    std::vector<SearchHit>& hits,
    const HitCallback& callback)
{
    ThreadContext ctx;
    ctx.scratch = seedfinder_scratch_create();
    if (ctx.scratch == nullptr) {
        return;
    }

    char error[256] = {};
    SeedProbeRequest req = buildProbeRequest(start, cfg, plan);
    for (std::uint64_t seed = start; seed <= end; ++seed) {
        if (cancel_flag != nullptr && cancel_flag->load()) {
            break;
        }

        req.seed = seed;
        SeedProbeResult result {};
        seedfinder::initDefaultResult(result);

        const int rc = seedfinder_probe(&req, ctx.scratch, &result, error, sizeof(error));
        if (rc != 0) {
            seedfinder_result_clear(&result);
            continue;
        }

        const ScoreResult score = scoreResult(cfg, result);
        ++checked;
        if (progress_counter != nullptr) {
            progress_counter->fetch_add(1, std::memory_order_relaxed);
        }
        if (score.partial_match_score > 0.f) {
            SearchHit hit;
            hit.seed = seed;
            hit.probe = result;
            hit.score = score;
            {
                std::lock_guard<std::mutex> lock(hit_mutex);
                hits.push_back(hit);
            }
            if (callback) {
                callback(hit);
            }
        } else {
            seedfinder_result_clear(&result);
        }
    }

    seedfinder_scratch_destroy(ctx.scratch);
}

} // namespace

SearchEngine::SearchEngine(config::SearchConfig cfg)
    : cfg_(std::move(cfg))
{
    checkpoint_.config_hash = hashConfig(cfg_);
    if (!cfg_.checkpoint_path.empty()) {
        loadCheckpoint(cfg_.checkpoint_path, checkpoint_);
    }
}

void SearchEngine::maintainTopHits(std::vector<SearchHit>& top, const SearchHit& hit)
{
    top.push_back(hit);
    std::sort(top.begin(), top.end(), [](const SearchHit& a, const SearchHit& b) {
        return a.score.partial_match_score > b.score.partial_match_score;
    });
    while (static_cast<int>(top.size()) > cfg_.top_k) {
        seedfinder_result_clear(&top.back().probe);
        top.pop_back();
    }
}

SearchSummary SearchEngine::run()
{
    seedfinder::runtime::initialize();

    SearchSummary summary;
    const int threadCount = std::max(1, cfg_.threads);
    const TierPlan plan = planTier(cfg_);

    std::uint64_t start = cfg_.seed_start;
    if (checkpoint_.cursor_seed > start) {
        start = checkpoint_.cursor_seed;
    }
    const std::uint64_t end = cfg_.seed_end;

    if (threadCount <= 1 || (end - start) < static_cast<std::uint64_t>(threadCount)) {
        ThreadContext ctx;
        ctx.scratch = seedfinder_scratch_create();
        char error[256] = {};
        SeedProbeRequest req = buildProbeRequest(start, cfg_, plan);

        for (std::uint64_t seed = start; seed <= end; ++seed) {
            if (cancel_flag_ != nullptr && cancel_flag_->load()) {
                break;
            }

            req.seed = seed;
            SeedProbeResult result {};
            seedfinder::initDefaultResult(result);

            if (seedfinder_probe(&req, ctx.scratch, &result, error, sizeof(error)) != 0) {
                seedfinder_result_clear(&result);
                checkpoint_.cursor_seed = seed + 1;
                continue;
            }

            const ScoreResult score = scoreResult(cfg_, result);
            ++summary.seeds_checked;
            if (progress_counter_ != nullptr) {
                progress_counter_->fetch_add(1, std::memory_order_relaxed);
            }
            checkpoint_.cursor_seed = seed + 1;

            if (score.partial_match_score > 0.f) {
                SearchHit hit {seed, result, score};
                maintainTopHits(summary.top_hits, hit);
                ++summary.hits_found;
                if (hit_callback_) {
                    hit_callback_(hit);
                }
            } else {
                seedfinder_result_clear(&result);
            }
        }

        seedfinder_scratch_destroy(ctx.scratch);
    } else {
        const std::uint64_t total = end - start + 1;
        const std::uint64_t chunk = total / static_cast<std::uint64_t>(threadCount);
        std::vector<std::thread> threads;
        std::vector<SearchHit> threadHits;
        std::mutex hitMutex;
        std::atomic<std::uint64_t> checked {0};

        for (int t = 0; t < threadCount; ++t) {
            const std::uint64_t tStart = start + static_cast<std::uint64_t>(t) * chunk;
            std::uint64_t tEnd = (t == threadCount - 1) ? end : (tStart + chunk - 1);
            threads.emplace_back(workerRange, std::cref(cfg_), std::cref(plan), tStart, tEnd, cancel_flag_, progress_counter_,
                std::ref(checked), std::ref(hitMutex), std::ref(threadHits), hit_callback_);
        }
        for (std::thread& th : threads) {
            th.join();
        }
        summary.seeds_checked = checked.load();
        for (SearchHit& hit : threadHits) {
            maintainTopHits(summary.top_hits, hit);
            ++summary.hits_found;
        }
        checkpoint_.cursor_seed = end + 1;
    }

    for (const SearchHit& hit : summary.top_hits) {
        checkpoint_.top_hits.push_back({hit.seed, hit.score.partial_match_score});
    }
    saveCheckpoint(cfg_.checkpoint_path, checkpoint_);
    return summary;
}

} // namespace seedfinder::engine
