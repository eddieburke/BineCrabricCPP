#include "net/minecraft/stat/AchievementMap.hpp"

#include <fstream>
#include <sstream>

#ifndef MINECRAFT_NATIVE_RESOURCE_DIR
#define MINECRAFT_NATIVE_RESOURCE_DIR "."
#endif

namespace net::minecraft::stat {

AchievementMap& AchievementMap::instance()
{
    static AchievementMap map;
    return map;
}

AchievementMap::AchievementMap()
{
    std::ifstream input(std::string(MINECRAFT_NATIVE_RESOURCE_DIR) + "/achievement/map.txt");
    if (!input.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::size_t comma = line.find(',');
        if (comma == std::string::npos) {
            continue;
        }
        const int statId = std::stoi(line.substr(0, comma));
        uuids_[statId] = line.substr(comma + 1);
    }
}

std::string AchievementMap::getUuid(int statId) const
{
    const auto it = uuids_.find(statId);
    return it != uuids_.end() ? it->second : std::string{};
}

} // namespace net::minecraft::stat
