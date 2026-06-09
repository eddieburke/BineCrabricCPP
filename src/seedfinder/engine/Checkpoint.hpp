#pragma once

#include "seedfinder/config/ConfigSchema.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace seedfinder::engine {

struct HitRecord {
    std::uint64_t seed = 0;
    float score = 0.f;
};

struct CheckpointState {
    std::uint64_t cursor_seed = 0;
    std::string config_hash;
    std::vector<HitRecord> top_hits;
};

[[nodiscard]] std::string hashConfig(const config::SearchConfig& cfg);
bool saveCheckpoint(const std::string& path, const CheckpointState& state);
bool loadCheckpoint(const std::string& path, CheckpointState& state);

} // namespace seedfinder::engine
