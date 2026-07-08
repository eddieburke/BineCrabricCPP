#pragma once
#include <string>
#include <unordered_map>

namespace net::minecraft::stat {
// Java AchievementMap loads /achievement/map.txt for Mojang stat UUIDs used by
// PlayerStats.serialize/deserialize checksum validation.
class AchievementMap {
   public:
    static AchievementMap& instance();
    [[nodiscard]] std::string getUuid(int statId) const;

   private:
    AchievementMap();
    std::unordered_map<int, std::string> uuids_;
};
}  // namespace net::minecraft::stat
