#pragma once
// Faithful 1:1 port of net.minecraft.item.ItemStack (beta 1.7.3).
//
// Kept value-semantic (the native port passes ItemStack by value through
// inventories and block entities). Item-dependent queries delegate to the Item
// registry via getItem(). Existing field names (itemId/count/damage) and helper
// methods (copy/split/toNbt/...) are preserved so current consumers keep working;
// faithful Java methods are added alongside.
#include <algorithm>
#include <cstdint>
#include <string>
#include "net/minecraft/block/BlockTypes.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft {
// Item is only forward-declared here: including Item.hpp would route the deep
// entity include graph (DataTracker -> ItemStack) back through Item and break
// the PlayerEntity definition cycle. Methods that dereference Item are defined
// out-of-line in ItemStack.cpp, which includes Item.hpp.
class Item;
class World;
namespace entity {
class Entity;
class LivingEntity;
} // namespace entity
namespace entity::player {
class PlayerEntity;
}
namespace block {
class Block;
}
struct ItemStack {
  int count = 0;
  int bobbingAnimationTime = 0;
  int itemId = 0;
  int damage = 0;
  constexpr ItemStack() = default;
  constexpr ItemStack(int id, int count = 1, int damage = 0) : count(count), itemId(id), damage(damage) {
  }
  // --- Item/Block constructors (faithful); defined in ItemStack.cpp ---
  explicit ItemStack(Item* item, int count = 1, int damage = 0);
  explicit ItemStack(Block* block, int count = 1, int damage = 0);
  // --- registry lookup + Item-backed queries (defined in ItemStack.cpp) ---
  [[nodiscard]] Item* getItem() const;
  [[nodiscard]] int getTextureId() const;
  [[nodiscard]] int getMaxCount() const;
  [[nodiscard]] int getMaxDamage() const;
  [[nodiscard]] bool isDamageable() const;
  [[nodiscard]] bool isDamaged() const;
  [[nodiscard]] bool hasSubtypes() const;
  [[nodiscard]] bool isStackable() const;
  [[nodiscard]] bool isSuitableFor(block::Block* block) const;
  [[nodiscard]] float getMiningSpeedMultiplier(block::Block* block) const;
  [[nodiscard]] int getAttackDamage(entity::Entity* attacked) const;
  [[nodiscard]] bool stackable() const; // existing native helper, Item-backed
  [[nodiscard]] std::string getTranslationKey() const;
  void applyDamage(int amount);
  void damageStack(int amount, entity::Entity* entity);
  void inventoryTick(World* world, entity::Entity* entity, int slot, bool selected);
  bool useOnBlock(entity::player::PlayerEntity* player, World* world, int x, int y, int z, int side);
  [[nodiscard]] ItemStack* use(World* world, entity::player::PlayerEntity* user);
  void postHit(entity::LivingEntity* target, entity::player::PlayerEntity* attacker);
  void postMine(int blockId, int x, int y, int z, entity::player::PlayerEntity* miner);
  void onCraft(World* world, entity::player::PlayerEntity* player);
  void onRemoved(entity::player::PlayerEntity* entity) const {
    (void)entity;
  }
  [[nodiscard]] int getDamage() const noexcept {
    return damage;
  }
  [[nodiscard]] int getDamage2() const noexcept {
    return damage;
  }
  void setDamage(int value) noexcept {
    damage = value;
  }
  [[nodiscard]] bool empty() const noexcept {
    return itemId == 0 || count <= 0;
  }
  // --- equality (faithful) ---
  [[nodiscard]] bool isItemEqual(const ItemStack& other) const noexcept {
    return itemId == other.itemId && damage == other.damage;
  }
  [[nodiscard]] bool sameItem(const ItemStack& other) const noexcept {
    return isItemEqual(other);
  }
  [[nodiscard]] bool equals(const ItemStack& other) const noexcept {
    return itemId == other.itemId && count == other.count && damage == other.damage;
  }
  [[nodiscard]] static bool areEqual(const ItemStack& left, const ItemStack& right) noexcept {
    return left.count == right.count && left.itemId == right.itemId && left.damage == right.damage;
  }
  [[nodiscard]] constexpr bool operator==(const ItemStack& other) const noexcept {
    return itemId == other.itemId && count == other.count && damage == other.damage;
  }
  [[nodiscard]] constexpr bool operator!=(const ItemStack& other) const noexcept {
    return !(*this == other);
  }
  // --- copy / split (faithful) ---
  [[nodiscard]] ItemStack copy() const noexcept {
    return {itemId, count, damage};
  }
  [[nodiscard]] ItemStack split(int amount) noexcept {
    amount = std::clamp(amount, 0, count);
    count -= amount;
    return {itemId, amount, damage};
  }
  // --- NBT (faithful field names) ---
  [[nodiscard]] NbtCompound toNbt() const {
    NbtCompound nbt;
    writeNbt(nbt);
    return nbt;
  }
  void writeNbt(NbtCompound& nbt) const {
    nbt.putShort("id", static_cast<std::int16_t>(itemId));
    nbt.putByte("Count", static_cast<std::int8_t>(count));
    nbt.putShort("Damage", static_cast<std::int16_t>(damage));
  }
  void readNbt(const NbtCompound& nbt) {
    itemId = nbt.getShort("id");
    count = nbt.getByte("Count");
    damage = nbt.getShort("Damage");
  }
  [[nodiscard]] static ItemStack fromNbt(const NbtCompound& nbt) {
    ItemStack stack;
    stack.readNbt(nbt);
    return stack;
  }
};
} // namespace net::minecraft
