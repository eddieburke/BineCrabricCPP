#include "net/minecraft/stat/Stats.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_set>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
namespace net::minecraft::stat {
namespace {
std::vector<StatId> g_mineBlockStats(256);
std::map<int, StatId> g_usedStats;
std::map<int, StatId> g_brokenStats;
std::map<int, StatId> g_craftedStats;
bool g_basicItemStatsInitialized = false;
bool g_extendedItemStatsInitialized = false;
void registerStat(StatId& stat, std::vector<StatId*>* categoryList = nullptr) {
 stat.uuid = AchievementMap::instance().getUuid(stat.id);
 if(stat.uuid.empty()) {
  stat.uuid = stat.name;
 }
 Stats::ID_TO_STAT[stat.id] = &stat;
 Stats::ALL_STATS.push_back(&stat);
 if(categoryList != nullptr) {
  categoryList->push_back(&stat);
 }
}
bool isStatInitialized(const StatId& stat) {
 return stat.id != 0;
}
void linkBlocks(std::vector<StatId>& statsArray, int sourceId, int targetId) {
 if(sourceId < 0 || targetId < 0 || sourceId >= static_cast<int>(statsArray.size()) ||
    targetId >= static_cast<int>(statsArray.size())) {
  return;
 }
 StatId& source = statsArray[static_cast<std::size_t>(sourceId)];
 StatId& target = statsArray[static_cast<std::size_t>(targetId)];
 if(!isStatInitialized(source)) {
  return;
 }
 if(!isStatInitialized(target)) {
  target = source;
  Stats::ID_TO_STAT[target.id] = &target;
  return;
 }
 auto removeStat = [&source](std::vector<StatId*>& list) {
  list.erase(std::remove(list.begin(), list.end(), &source), list.end());
 };
 removeStat(Stats::ALL_STATS);
 removeStat(Stats::BLOCK_MINED_STATS);
 removeStat(Stats::GENERAL_STATS);
 removeStat(Stats::ITEM_STATS);
 Stats::ID_TO_STAT.erase(source.id);
 source = target;
 Stats::ID_TO_STAT[source.id] = &source;
}
void linkBlocks(std::map<int, StatId>& statsByItemId, int sourceId, int targetId) {
 auto sourceIt = statsByItemId.find(sourceId);
 if(sourceIt == statsByItemId.end() || !isStatInitialized(sourceIt->second)) {
  return;
 }
 auto targetIt = statsByItemId.find(targetId);
 if(targetIt == statsByItemId.end() || !isStatInitialized(targetIt->second)) {
  StatId& target = statsByItemId[targetId];
  target = sourceIt->second;
  Stats::ID_TO_STAT[target.id] = &target;
  return;
 }
 StatId& source = sourceIt->second;
 StatId& target = targetIt->second;
 auto removeStat = [&source](std::vector<StatId*>& list) {
  list.erase(std::remove(list.begin(), list.end(), &source), list.end());
 };
 removeStat(Stats::ALL_STATS);
 removeStat(Stats::BLOCK_MINED_STATS);
 removeStat(Stats::GENERAL_STATS);
 removeStat(Stats::ITEM_STATS);
 Stats::ID_TO_STAT.erase(source.id);
 source = target;
 Stats::ID_TO_STAT[source.id] = &source;
}
void linkAlternateBlockStats(std::vector<StatId>& statsArray) {
 using block::Block;
 if(Block::WATER != nullptr && Block::FLOWING_WATER != nullptr) {
  linkBlocks(statsArray, Block::WATER->id, Block::FLOWING_WATER->id);
 }
 if(Block::LAVA != nullptr) {
  linkBlocks(statsArray, Block::LAVA->id, Block::LAVA->id);
 }
 if(Block::JACK_O_LANTERN != nullptr && Block::PUMPKIN != nullptr) {
  linkBlocks(statsArray, Block::JACK_O_LANTERN->id, Block::PUMPKIN->id);
 }
 if(Block::LIT_FURNACE != nullptr && Block::FURNACE != nullptr) {
  linkBlocks(statsArray, Block::LIT_FURNACE->id, Block::FURNACE->id);
 }
 if(Block::LIT_REDSTONE_ORE != nullptr && Block::REDSTONE_ORE != nullptr) {
  linkBlocks(statsArray, Block::LIT_REDSTONE_ORE->id, Block::REDSTONE_ORE->id);
 }
 if(Block::POWERED_REPEATER != nullptr && Block::REPEATER != nullptr) {
  linkBlocks(statsArray, Block::POWERED_REPEATER->id, Block::REPEATER->id);
 }
 if(Block::LIT_REDSTONE_TORCH != nullptr && Block::REDSTONE_TORCH != nullptr) {
  linkBlocks(statsArray, Block::LIT_REDSTONE_TORCH->id, Block::REDSTONE_TORCH->id);
 }
 if(Block::RED_MUSHROOM != nullptr && Block::BROWN_MUSHROOM != nullptr) {
  linkBlocks(statsArray, Block::RED_MUSHROOM->id, Block::BROWN_MUSHROOM->id);
 }
 if(Block::DOUBLE_SLAB != nullptr && Block::SLAB != nullptr) {
  linkBlocks(statsArray, Block::DOUBLE_SLAB->id, Block::SLAB->id);
 }
 if(Block::GRASS_BLOCK != nullptr && Block::DIRT != nullptr) {
  linkBlocks(statsArray, Block::GRASS_BLOCK->id, Block::DIRT->id);
 }
 if(Block::FARMLAND != nullptr && Block::DIRT != nullptr) {
  linkBlocks(statsArray, Block::FARMLAND->id, Block::DIRT->id);
 }
}
void linkAlternateBlockStats(std::map<int, StatId>& statsByItemId) {
 using block::Block;
 if(Block::WATER != nullptr && Block::FLOWING_WATER != nullptr) {
  linkBlocks(statsByItemId, Block::WATER->id, Block::FLOWING_WATER->id);
 }
 if(Block::LAVA != nullptr) {
  linkBlocks(statsByItemId, Block::LAVA->id, Block::LAVA->id);
 }
 if(Block::JACK_O_LANTERN != nullptr && Block::PUMPKIN != nullptr) {
  linkBlocks(statsByItemId, Block::JACK_O_LANTERN->id, Block::PUMPKIN->id);
 }
 if(Block::LIT_FURNACE != nullptr && Block::FURNACE != nullptr) {
  linkBlocks(statsByItemId, Block::LIT_FURNACE->id, Block::FURNACE->id);
 }
 if(Block::LIT_REDSTONE_ORE != nullptr && Block::REDSTONE_ORE != nullptr) {
  linkBlocks(statsByItemId, Block::LIT_REDSTONE_ORE->id, Block::REDSTONE_ORE->id);
 }
 if(Block::POWERED_REPEATER != nullptr && Block::REPEATER != nullptr) {
  linkBlocks(statsByItemId, Block::POWERED_REPEATER->id, Block::REPEATER->id);
 }
 if(Block::LIT_REDSTONE_TORCH != nullptr && Block::REDSTONE_TORCH != nullptr) {
  linkBlocks(statsByItemId, Block::LIT_REDSTONE_TORCH->id, Block::REDSTONE_TORCH->id);
 }
 if(Block::RED_MUSHROOM != nullptr && Block::BROWN_MUSHROOM != nullptr) {
  linkBlocks(statsByItemId, Block::RED_MUSHROOM->id, Block::BROWN_MUSHROOM->id);
 }
 if(Block::DOUBLE_SLAB != nullptr && Block::SLAB != nullptr) {
  linkBlocks(statsByItemId, Block::DOUBLE_SLAB->id, Block::SLAB->id);
 }
 if(Block::GRASS_BLOCK != nullptr && Block::DIRT != nullptr) {
  linkBlocks(statsByItemId, Block::GRASS_BLOCK->id, Block::DIRT->id);
 }
 if(Block::FARMLAND != nullptr && Block::DIRT != nullptr) {
  linkBlocks(statsByItemId, Block::FARMLAND->id, Block::DIRT->id);
 }
}
void initBlocksMined() {
 for(int blockId = 0; blockId < block::Block::BLOCK_COUNT; ++blockId) {
  block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block == nullptr || !block->isTrackingStatistics()) {
   continue;
  }
  StatId& stat = g_mineBlockStats[static_cast<std::size_t>(blockId)];
  stat.id = Stats::MINE_BLOCK_BASE + blockId;
  stat.name = "stat.mineBlock." + block->getTranslationKey();
  stat.itemOrBlockId = blockId;
  stat.formatter = StatFormatterKind::Integer;
  registerStat(stat, &Stats::BLOCK_MINED_STATS);
 }
 linkAlternateBlockStats(g_mineBlockStats);
}
void initItemsUsedStats(int minId, int maxId) {
 for(int itemId = minId; itemId < maxId; ++itemId) {
  Item* item = Item::ITEMS[static_cast<std::size_t>(itemId)];
  if(item == nullptr) {
   continue;
  }
  StatId& stat = g_usedStats[itemId];
  stat.id = Stats::USED_BASE + itemId;
  stat.name = "stat.useItem." + item->getTranslationKey();
  stat.itemOrBlockId = itemId;
  stat.formatter = StatFormatterKind::Integer;
  registerStat(stat, itemId >= block::Block::BLOCK_COUNT ? &Stats::ITEM_STATS : nullptr);
 }
 linkAlternateBlockStats(g_usedStats);
}
void initBrokenItemStats(int start, int end) {
 for(int itemId = start; itemId < end; ++itemId) {
  Item* item = Item::ITEMS[static_cast<std::size_t>(itemId)];
  if(item == nullptr || !item->isDamageable()) {
   continue;
  }
  StatId& stat = g_brokenStats[itemId];
  stat.id = Stats::BROKEN_BASE + itemId;
  stat.name = "stat.breakItem." + item->getTranslationKey();
  stat.itemOrBlockId = itemId;
  stat.formatter = StatFormatterKind::Integer;
  registerStat(stat);
 }
 linkAlternateBlockStats(g_brokenStats);
}
void initializeCraftedItemStats() {
 if(!g_basicItemStatsInitialized || !g_extendedItemStatsInitialized) {
  return;
 }
 std::unordered_set<int> craftedIds;
 for(const auto& recipe : recipe::CraftingRecipeManager::getInstance().getRecipes()) {
  if(recipe != nullptr && recipe->getOutput().itemId > 0) {
   craftedIds.insert(recipe->getOutput().itemId);
  }
 }
 for(const auto& [inputId, output] : recipe::SmeltingRecipeManager::instance().getRecipes()) {
  (void)inputId;
  if(output.itemId > 0) {
   craftedIds.insert(output.itemId);
  }
 }
 for(const int itemId : craftedIds) {
  Item* item = Item::ITEMS[static_cast<std::size_t>(itemId)];
  if(item == nullptr) {
   continue;
  }
  StatId& stat = g_craftedStats[itemId];
  stat.id = Stats::CRAFTED_BASE + itemId;
  stat.name = "stat.craftItem." + item->getTranslationKey();
  stat.itemOrBlockId = itemId;
  stat.formatter = StatFormatterKind::Integer;
  registerStat(stat);
 }
 linkAlternateBlockStats(g_craftedStats);
}
std::once_flag g_statsInit;
} // namespace
void Stats::initializeOnce() {
 if(!ALL_STATS.empty()) {
  return;
 }
 START_GAME.local = true;
 CREATE_WORLD.local = true;
 LOAD_WORLD.local = true;
 JOIN_MULTIPLAYER.local = true;
 LEAVE_GAME.local = true;
 PLAY_ONE_MINUTE.local = true;
 WALK_ONE_CM.local = true;
 SWIM_ONE_CM.local = true;
 FALL_ONE_CM.local = true;
 CLIMB_ONE_CM.local = true;
 FLY_ONE_CM.local = true;
 DIVE_ONE_CM.local = true;
 MINECART_ONE_CM.local = true;
 BOAT_ONE_CM.local = true;
 PIG_ONE_CM.local = true;
 JUMP.local = true;
 DROP.local = true;
 registerGeneral(START_GAME);
 registerGeneral(CREATE_WORLD);
 registerGeneral(LOAD_WORLD);
 registerGeneral(JOIN_MULTIPLAYER);
 registerGeneral(LEAVE_GAME);
 registerGeneral(PLAY_ONE_MINUTE);
 registerGeneral(WALK_ONE_CM);
 registerGeneral(SWIM_ONE_CM);
 registerGeneral(FALL_ONE_CM);
 registerGeneral(CLIMB_ONE_CM);
 registerGeneral(FLY_ONE_CM);
 registerGeneral(DIVE_ONE_CM);
 registerGeneral(MINECART_ONE_CM);
 registerGeneral(BOAT_ONE_CM);
 registerGeneral(PIG_ONE_CM);
 registerGeneral(JUMP);
 registerGeneral(DROP);
 registerGeneral(DAMAGE_DEALT);
 registerGeneral(DAMAGE_TAKEN);
 registerGeneral(DEATHS);
 registerGeneral(MOB_KILLS);
 registerGeneral(PLAYER_KILLS);
 registerGeneral(FISH_CAUGHT);
 registerAchievements();
 initBlocksMined();
 initializeItemStats();
 initializeExtendedItemStats();
}
void Stats::initializeItemStats() {
 if(g_basicItemStatsInitialized) {
  return;
 }
 initItemsUsedStats(0, block::Block::BLOCK_COUNT);
 initBrokenItemStats(0, block::Block::BLOCK_COUNT);
 g_basicItemStatsInitialized = true;
 initializeCraftedItemStats();
}
void Stats::initializeExtendedItemStats() {
 if(g_extendedItemStatsInitialized) {
  return;
 }
 initItemsUsedStats(block::Block::BLOCK_COUNT, Item::ITEM_COUNT);
 initBrokenItemStats(block::Block::BLOCK_COUNT, Item::ITEM_COUNT);
 g_extendedItemStatsInitialized = true;
 initializeCraftedItemStats();
}
StatId* Stats::getMineBlockStat(int blockId) {
 initialize();
 if(blockId < 0 || blockId >= static_cast<int>(g_mineBlockStats.size())) {
  return nullptr;
 }
 StatId& stat = g_mineBlockStats[static_cast<std::size_t>(blockId)];
 return stat.id != 0 ? &stat : nullptr;
}
StatId* Stats::getUsedStat(int itemId) {
 initialize();
 auto it = g_usedStats.find(itemId);
 if(it == g_usedStats.end() || it->second.id == 0) {
  return nullptr;
 }
 return &it->second;
}
StatId* Stats::getBrokenStat(int itemId) {
 initialize();
 auto it = g_brokenStats.find(itemId);
 if(it == g_brokenStats.end() || it->second.id == 0) {
  return nullptr;
 }
 return &it->second;
}
StatId* Stats::getCraftedStat(int itemId) {
 initialize();
 auto it = g_craftedStats.find(itemId);
 if(it == g_craftedStats.end() || it->second.id == 0) {
  return nullptr;
 }
 return &it->second;
}
void Stats::registerGeneral(StatId& stat) {
 if(stat.id == PLAY_ONE_MINUTE.id) {
  stat.formatter = StatFormatterKind::Time;
 } else if(stat.id >= WALK_ONE_CM.id && stat.id <= PIG_ONE_CM.id) {
  stat.formatter = StatFormatterKind::Distance;
 } else {
  stat.formatter = StatFormatterKind::Integer;
 }
 stat.uuid = AchievementMap::instance().getUuid(stat.id);
 if(stat.uuid.empty()) {
  stat.uuid = stat.name;
 }
 Stats::ID_TO_STAT[stat.id] = &stat;
 Stats::ALL_STATS.push_back(&stat);
 Stats::GENERAL_STATS.push_back(&stat);
}
void Stats::registerAchievements() {
 static std::vector<StatId> storage;
 if(!storage.empty()) {
  return;
 }
 for(const achievement::AchievementDef& achievement : achievement::Achievements::ALL) {
  StatId stat;
  stat.id = achievement.statId();
  stat.name = std::string("achievement.") + achievement.key;
  stat.uuid = AchievementMap::instance().getUuid(achievement.statId());
  stat.local = achievement.localOnly;
  stat.achievement = true;
  stat.parentStatId = achievement.parentStatId();
  storage.push_back(std::move(stat));
  StatId& statRef = storage.back();
  if(statRef.uuid.empty()) {
   statRef.uuid = statRef.name;
  }
  Stats::ID_TO_STAT[statRef.id] = &statRef;
  Stats::ALL_STATS.push_back(&statRef);
 }
}
void Stats::initialize() {
 std::call_once(g_statsInit, &Stats::initializeOnce);
}
StatId* Stats::getStatById(int id) {
 initialize();
 auto it = Stats::ID_TO_STAT.find(id);
 return it != Stats::ID_TO_STAT.end() ? it->second : nullptr;
}
bool Stats::isLocalOnly(int statId) {
 if(StatId* stat = getStatById(statId)) {
  return stat->local;
 }
 if(const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(statId)) {
  return achievement->localOnly;
 }
 return false;
}
} // namespace net::minecraft::stat
