#pragma once
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/stat/AchievementMap.hpp"
#include "net/minecraft/stat/StatFormatter.hpp"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace net::minecraft::stat {
struct StatId {
  int id = 0;
  std::string name;
  std::string uuid;
  bool local = false;
  bool achievement = false;
  int parentStatId = -1;
  int itemOrBlockId = -1;
  StatFormatterKind formatter = StatFormatterKind::Integer;
  StatId() = default;
  StatId(int id_, std::string name_) : id(id_), name(std::move(name_)) {}
  [[nodiscard]] bool operator==(const StatId& other) const noexcept {
    return id == other.id;
  }
  [[nodiscard]] std::string format(int value) const {
    return formatStatValue(formatter, value);
  }
};
struct Stats {
  static constexpr int MINE_BLOCK_BASE = 0x1000000;
  static constexpr int CRAFTED_BASE = 0x1010000;
  static constexpr int USED_BASE = 0x1020000;
  static constexpr int BROKEN_BASE = 0x1030000;
  static constexpr int ACHIEVEMENT_BASE = achievement::AchievementDef::kBase;
  inline static std::unordered_map<int, StatId*> ID_TO_STAT{};
  inline static std::vector<StatId*> ALL_STATS{};
  inline static std::vector<StatId*> GENERAL_STATS{};
  inline static std::vector<StatId*> ITEM_STATS{};
  inline static std::vector<StatId*> BLOCK_MINED_STATS{};
  inline static StatId START_GAME{1000, "stat.startGame"};
  inline static StatId CREATE_WORLD{1001, "stat.createWorld"};
  inline static StatId LOAD_WORLD{1002, "stat.loadWorld"};
  inline static StatId JOIN_MULTIPLAYER{1003, "stat.joinMultiplayer"};
  inline static StatId LEAVE_GAME{1004, "stat.leaveGame"};
  inline static StatId PLAY_ONE_MINUTE{1100, "stat.playOneMinute"};
  inline static StatId WALK_ONE_CM{2000, "stat.walkOneCm"};
  inline static StatId SWIM_ONE_CM{2001, "stat.swimOneCm"};
  inline static StatId FALL_ONE_CM{2002, "stat.fallOneCm"};
  inline static StatId CLIMB_ONE_CM{2003, "stat.climbOneCm"};
  inline static StatId FLY_ONE_CM{2004, "stat.flyOneCm"};
  inline static StatId DIVE_ONE_CM{2005, "stat.diveOneCm"};
  inline static StatId MINECART_ONE_CM{2006, "stat.minecartOneCm"};
  inline static StatId BOAT_ONE_CM{2007, "stat.boatOneCm"};
  inline static StatId PIG_ONE_CM{2008, "stat.pigOneCm"};
  inline static StatId JUMP{2010, "stat.jump"};
  inline static StatId DROP{2011, "stat.drop"};
  inline static StatId DAMAGE_DEALT{2020, "stat.damageDealt"};
  inline static StatId DAMAGE_TAKEN{2021, "stat.damageTaken"};
  inline static StatId DEATHS{2022, "stat.deaths"};
  inline static StatId MOB_KILLS{2023, "stat.mobKills"};
  inline static StatId PLAYER_KILLS{2024, "stat.playerKills"};
  inline static StatId FISH_CAUGHT{2025, "stat.fishCaught"};
  static void initialize();
  static void initializeItemStats();
  static void initializeExtendedItemStats();
  [[nodiscard]] static int mineBlockStatId(int blockId) noexcept {
    return MINE_BLOCK_BASE + blockId;
  }
  [[nodiscard]] static int craftedItemStatId(int itemId) noexcept {
    return CRAFTED_BASE + itemId;
  }
  [[nodiscard]] static int usedItemStatId(int itemId) noexcept {
    return USED_BASE + itemId;
  }
  [[nodiscard]] static int brokenItemStatId(int itemId) noexcept {
    return BROKEN_BASE + itemId;
  }
  [[nodiscard]] static StatId* getStatById(int id);
  [[nodiscard]] static StatId* getMineBlockStat(int blockId);
  [[nodiscard]] static StatId* getUsedStat(int itemId);
  [[nodiscard]] static StatId* getBrokenStat(int itemId);
  [[nodiscard]] static StatId* getCraftedStat(int itemId);
  [[nodiscard]] static bool isLocalOnly(int statId);

private:
  static void initializeOnce();
  static void registerGeneral(StatId& stat);
  static void registerAchievements();
};
} // namespace net::minecraft::stat
