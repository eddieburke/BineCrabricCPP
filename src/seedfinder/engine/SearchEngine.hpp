#pragma once

#include "seedfinder/config/ConfigSchema.hpp"
#include "seedfinder/engine/Checkpoint.hpp"
#include "seedfinder/engine/Scorer.hpp"

#include <atomic>
#include <functional>
#include <vector>

namespace seedfinder::engine {

struct SearchHit {
    std::uint64_t seed = 0;
    SeedProbeResult probe {};
    ScoreResult score {};
};

struct SearchSummary {
    std::uint64_t seeds_checked = 0;
    std::uint64_t hits_found = 0;
    std::vector<SearchHit> top_hits;
};

using HitCallback = std::function<void(const SearchHit&)>;

class SearchEngine {
public:
    explicit SearchEngine(config::SearchConfig cfg);

    void setCancelFlag(std::atomic<bool>* flag) { cancel_flag_ = flag; }
    void setHitCallback(HitCallback callback) { hit_callback_ = std::move(callback); }
    void setProgressCounter(std::atomic<std::uint64_t>* counter) { progress_counter_ = counter; }

    [[nodiscard]] SearchSummary run();

private:
    config::SearchConfig cfg_;
    std::atomic<bool>* cancel_flag_ = nullptr;
    std::atomic<std::uint64_t>* progress_counter_ = nullptr;
    HitCallback hit_callback_;
    CheckpointState checkpoint_;

    void maintainTopHits(std::vector<SearchHit>& top, const SearchHit& hit);
};

} // namespace seedfinder::engine
