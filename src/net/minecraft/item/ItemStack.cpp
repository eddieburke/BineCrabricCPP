#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/World.hpp"
// Out-of-line bodies for ItemStack methods that dereference the Item registry.
// Kept here (not in the header) so ItemStack.hpp does not pull Item.hpp into the
// entity/DataTracker include graph, which would re-enter PlayerEntity mid-parse.
namespace net::minecraft {
ItemStack::ItemStack(Item* item, int count, int damage)
    : count(count), itemId(item != nullptr ? item->id : 0), damage(damage) {
}
ItemStack::ItemStack(Block* block, int count, int damage)
    : count(count), itemId(block != nullptr ? block->id : 0), damage(damage) {
}
Item* ItemStack::getItem() const {
 if(itemId < 0 || itemId >= Item::ITEM_COUNT) {
  return nullptr;
 }
 return Item::ITEMS[static_cast<std::size_t>(itemId)];
}
int ItemStack::getTextureId() const {
 Item* item = getItem();
 return item != nullptr ? item->getTextureId(damage) : 0;
}
int ItemStack::getMaxCount() const {
 Item* item = getItem();
 return item != nullptr ? item->getMaxCount() : 64;
}
int ItemStack::getMaxDamage() const {
 Item* item = getItem();
 return item != nullptr ? item->getMaxDamage() : 0;
}
bool ItemStack::isDamageable() const {
 return getMaxDamage() > 0;
}
bool ItemStack::isDamaged() const {
 return isDamageable() && damage > 0;
}
bool ItemStack::hasSubtypes() const {
 Item* item = getItem();
 return item != nullptr && item->hasSubtypes();
}
bool ItemStack::isStackable() const {
 return getMaxCount() > 1 && (!isDamageable() || !isDamaged());
}
bool ItemStack::isSuitableFor(block::Block* block) const {
 Item* item = getItem();
 return item != nullptr && block != nullptr && item->isSuitableFor(block);
}
float ItemStack::getMiningSpeedMultiplier(block::Block* block) const {
 Item* item = getItem();
 if(item == nullptr || block == nullptr) {
  return 1.0f;
 }
 return item->getMiningSpeedMultiplier(const_cast<ItemStack*>(this), block);
}
bool ItemStack::stackable() const {
 return !empty() && count < getMaxCount();
}
int ItemStack::getAttackDamage(entity::Entity* attacked) const {
 Item* item = getItem();
 if(item == nullptr) {
  return 1;
 }
 return item->getAttackDamage(attacked);
}
std::string ItemStack::getTranslationKey() const {
 Item* item = getItem();
 return item != nullptr ? item->getTranslationKey(this) : std::string{};
}
void ItemStack::applyDamage(int amount) {
 damageStack(amount, nullptr);
}
void ItemStack::damageStack(int amount, entity::Entity* entity) {
 if(!isDamageable()) {
  return;
 }
 this->damage += amount;
 if(this->damage > getMaxDamage()) {
  if(auto* player = dynamic_cast<entity::player::PlayerEntity*>(entity)) {
   player->increaseStat(stat::Stats::brokenItemStatId(itemId), 1);
  }
  --count;
  if(count < 0) {
   count = 0;
  }
  this->damage = 0;
 }
}
void ItemStack::inventoryTick(World* world, entity::Entity* entity, int slot, bool selected) {
 if(bobbingAnimationTime > 0) {
  --bobbingAnimationTime;
 }
 Item* item = getItem();
 if(item != nullptr) {
  item->inventoryTick(this, world, entity, slot, selected);
 }
}
bool ItemStack::useOnBlock(entity::player::PlayerEntity* player, World* world, int x, int y, int z, int side) {
 Item* item = getItem();
 if(item == nullptr) {
  return false;
 }
 if(item->useOnBlock(this, player, world, x, y, z, side)) {
  if(player != nullptr) {
   player->increaseStat(stat::Stats::usedItemStatId(itemId), 1);
  }
  return true;
 }
 return false;
}
ItemStack* ItemStack::use(World* world, entity::player::PlayerEntity* user) {
 Item* item = getItem();
 return item != nullptr ? item->use(this, world, user) : nullptr;
}
void ItemStack::postMine(int blockId, int x, int y, int z, entity::player::PlayerEntity* miner) {
 Item* item = getItem();
 if(item == nullptr || miner == nullptr) {
  return;
 }
 if(item->postMine(this, blockId, x, y, z, miner)) {
  miner->increaseStat(stat::Stats::usedItemStatId(itemId), 1);
 }
}
void ItemStack::postHit(entity::LivingEntity* target, entity::player::PlayerEntity* attacker) {
 Item* item = getItem();
 if(item == nullptr || target == nullptr || attacker == nullptr) {
  return;
 }
 if(item->postHit(this, target, attacker)) {
  attacker->increaseStat(stat::Stats::usedItemStatId(itemId), 1);
 }
}
void ItemStack::onCraft(World* world, entity::player::PlayerEntity* player) {
 if(player != nullptr) {
  player->increaseStat(stat::Stats::craftedItemStatId(itemId), count);
 }
 Item* item = getItem();
 if(item != nullptr) {
  item->onCraft(this, world, player);
 }
}
} // namespace net::minecraft
