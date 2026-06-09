#pragma once

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/stat/Stats.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace net::minecraft::stat {

class PlayerStats {
public:
    PlayerStats(const client::util::Session& session, std::filesystem::path runDirectory);
    ~PlayerStats();

    void increment(const StatId& stat, int amount);
    void incrementById(int statId, int amount);
    void increment(const std::unordered_map<int, int>& amounts);

    void tick(const std::unordered_map<int, int>& remoteStats);
    void tickUnsent(const std::unordered_map<int, int>& amounts);

    [[nodiscard]] int get(const StatId& stat) const { return getFrom(stats_, stat.id); }
    [[nodiscard]] int getById(int statId) const { return getFrom(stats_, statId); }
    [[nodiscard]] bool hasStat(int statId) const { return getFrom(stats_, statId) > 0; }
    [[nodiscard]] bool hasAchievement(const StatId& stat) const { return hasStat(stat.id); }
    [[nodiscard]] bool hasParentAchievement(int statId) const;

    void syncStats() {}

    void save();
    void tick();

private:
    static void doIncrement(std::unordered_map<int, int>& target, int statId, int amount);
    [[nodiscard]] static int getFrom(const std::unordered_map<int, int>& source, int statId);
    void relocateLegacyStats() const;
    void loadFromDisk();
    [[nodiscard]] static std::unordered_map<int, int> importLegacyJson(const std::string& data);
    [[nodiscard]] std::string usernameKey() const;

    client::util::Session session_;
    std::filesystem::path runDirectory_;
    std::filesystem::path statsDirectory_;
    std::filesystem::path statsFile_;
    std::unordered_map<int, int> stats_;
    bool dirty_ = false;
    int saveCooldown_ = 0;
};

} // namespace net::minecraft::stat
