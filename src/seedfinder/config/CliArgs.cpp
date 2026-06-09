#include "seedfinder/config/CliArgs.hpp"

#include "seedfinder/engine/SeedString.hpp"

#include <iostream>

namespace seedfinder::config {

CliOverrides parseCliArgs(int argc, char** argv)
{
    CliOverrides out;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            out.show_help = true;
            return out;
        }
        if (arg == "--config" && i + 1 < argc) {
            out.has_config_path = true;
            out.config_path = argv[++i];
            continue;
        }
        if (arg == "--threads" && i + 1 < argc) {
            out.has_threads = true;
            out.threads = std::stoi(argv[++i]);
            continue;
        }
        if (arg == "--seed-start" && i + 1 < argc) {
            out.has_seed_start = true;
            out.seed_start = parseSeedToken(argv[++i]);
            continue;
        }
        if (arg == "--seed-end" && i + 1 < argc) {
            out.has_seed_end = true;
            out.seed_end = parseSeedToken(argv[++i]);
            continue;
        }
        if (arg == "--top" && i + 1 < argc) {
            out.has_top_k = true;
            out.top_k = std::stoi(argv[++i]);
            continue;
        }
        if (arg == "--checkpoint" && i + 1 < argc) {
            out.has_checkpoint = true;
            out.checkpoint_path = argv[++i];
            continue;
        }
    }
    return out;
}

void applyCliOverrides(SearchConfig& cfg, const CliOverrides& overrides)
{
    if (overrides.has_threads) {
        cfg.threads = overrides.threads;
    }
    if (overrides.has_seed_start) {
        cfg.seed_start = overrides.seed_start;
    }
    if (overrides.has_seed_end) {
        cfg.seed_end = overrides.seed_end;
    }
    if (overrides.has_top_k) {
        cfg.top_k = overrides.top_k;
    }
    if (overrides.has_checkpoint) {
        cfg.checkpoint_path = overrides.checkpoint_path;
    }
}

void printCliHelp()
{
    std::cout
        << "seedfinder — Beta 1.7.3 overworld seed search (v1)\n"
        << "Usage: seedfinder --config <path.json> [options]\n"
        << "  --config <path>       JSON config file (required)\n"
        << "  --threads <n>         worker threads (default 1)\n"
        << "  --seed-start <seed>   override seed_range_start\n"
        << "  --seed-end <seed>     override seed_range_end\n"
        << "  --top <k>             keep top K hits\n"
        << "  --checkpoint <path>   checkpoint file path\n"
        << "  --help                show this help\n";
}

} // namespace seedfinder::config
