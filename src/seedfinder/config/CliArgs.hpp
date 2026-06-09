#pragma once

#include "seedfinder/config/ConfigSchema.hpp"

#include <string>

namespace seedfinder::config {

struct CliOverrides {
    bool has_config_path = false;
    std::string config_path;
    bool has_threads = false;
    int threads = 1;
    bool has_seed_start = false;
    std::uint64_t seed_start = 0;
    bool has_seed_end = false;
    std::uint64_t seed_end = 0;
    bool has_top_k = false;
    int top_k = 10;
    bool has_checkpoint = false;
    std::string checkpoint_path;
    bool show_help = false;
};

[[nodiscard]] CliOverrides parseCliArgs(int argc, char** argv);
void applyCliOverrides(SearchConfig& cfg, const CliOverrides& overrides);
void printCliHelp();

} // namespace seedfinder::config
