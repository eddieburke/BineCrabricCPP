#include "seedfinder/config/CliArgs.hpp"
#include "seedfinder/config/JsonConfig.hpp"
#include "seedfinder/engine/SearchEngine.hpp"
#include "seedfinder/runtime/RuntimeInit.hpp"

#include <iostream>
#include <sstream>

namespace {

void printHitNdjson(const seedfinder::engine::SearchHit& hit)
{
    std::ostringstream line;
    line << "{\"seed\":\"" << hit.seed << "\""
         << ",\"score\":" << hit.score.partial_match_score
         << ",\"spawn_x\":" << hit.probe.spawn.x
         << ",\"spawn_z\":" << hit.probe.spawn.z
         << ",\"spawn_biome\":" << static_cast<int>(hit.probe.spawn.biome_id)
         << ",\"dominant_biome\":" << static_cast<int>(hit.probe.dominant_biome_id)
         << ",\"avg_surface_y\":" << hit.probe.avg_surface_y
         << ",\"underwater_percent\":" << hit.probe.underwater_percent
         << ",\"tier\":" << static_cast<int>(hit.score.search_tier_passed)
         << ",\"all_hard\":" << (hit.score.all_hard_constraints_met ? "true" : "false")
         << '}';
    std::cout << line.str() << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    using namespace seedfinder::config;
    using namespace seedfinder::engine;

    const CliOverrides cli = parseCliArgs(argc, argv);
    if (cli.show_help) {
        printCliHelp();
        return 0;
    }
    if (!cli.has_config_path) {
        printCliHelp();
        return 1;
    }

    LoadResult loaded = loadConfigFromFile(cli.config_path);
    if (!loaded.ok) {
        std::cerr << "config error: " << loaded.error << '\n';
        return 1;
    }
    applyCliOverrides(loaded.config, cli);

    seedfinder::runtime::initialize();

    SearchEngine engine(std::move(loaded.config));
    engine.setHitCallback(printHitNdjson);
    const SearchSummary summary = engine.run();

    std::cout << "{\"summary\":true"
              << ",\"seeds_checked\":" << summary.seeds_checked
              << ",\"hits_found\":" << summary.hits_found
              << ",\"top_k\":" << summary.top_hits.size()
              << "}\n";

    seedfinder::runtime::shutdown();
    return 0;
}
