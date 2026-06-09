#include "seedfinder/engine/Checkpoint.hpp"

#include <fstream>
#include <sstream>

namespace seedfinder::engine {

std::string hashConfig(const config::SearchConfig& cfg)
{
    std::ostringstream out;
    out << cfg.seed_start << ':' << cfg.seed_end << ':' << cfg.dimension << ':' << cfg.scan_radius_chunks;
    return out.str();
}

bool saveCheckpoint(const std::string& path, const CheckpointState& state)
{
    if (path.empty()) {
        return true;
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "{\n";
    out << "  \"cursor_seed\": \"" << state.cursor_seed << "\",\n";
    out << "  \"config_hash\": \"" << state.config_hash << "\",\n";
    out << "  \"top_hits\": [\n";
    for (std::size_t i = 0; i < state.top_hits.size(); ++i) {
        const HitRecord& hit = state.top_hits[i];
        out << "    {\"seed\": \"" << hit.seed << "\", \"score\": " << hit.score << "}";
        if (i + 1 < state.top_hits.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
    return true;
}

bool loadCheckpoint(const std::string& path, CheckpointState& state)
{
    if (path.empty()) {
        return true;
    }
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    const std::size_t cursorPos = text.find("cursor_seed");
    if (cursorPos != std::string::npos) {
        const std::size_t quoteStart = text.find('"', cursorPos + 12);
        const std::size_t quoteEnd = text.find('"', quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            state.cursor_seed = std::stoull(text.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
        }
    }
    return true;
}

} // namespace seedfinder::engine
