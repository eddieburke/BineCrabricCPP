#pragma once
// Faithful 1:1 port of net.minecraft.item.Item base class (beta 1.7.3).
//
// Item participates in a dependency cycle (Item <-> Block <-> ItemStack <->
// World/Entity). The base class only needs forward declarations; concrete item
// Vanilla items register via per-TU RegisterItem during Registry::bootstrap().
//
// ITEMS is a flat array of owning raw pointers, mirroring Java's static Item[].
// Like the Java statics these are never freed (process-lifetime singletons).
#include <array>
#include <cstdint>
#include <string>
#include "net/minecraft/util/math/Types.hpp" // JavaRandom
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/block/BlockTypes.hpp"
namespace net::minecraft {
// Forward declarations break the cycle; method bodies that need the full types
// are defined out-of-line in translation units that include those headers.
class World;
class ItemStack;
enum class RegistrationMode { Immediate,
                              Deferred };
class Item {
public:
  static constexpr int ITEM_COUNT = 32000;
  static std::array<Item*, ITEM_COUNT> ITEMS;
  static JavaRandom random;
  [[nodiscard]] static Item* byRawId(int rawId) {
    return ITEMS[static_cast<std::size_t>(256 + rawId)];
  }
  [[nodiscard]] static Item* byId(int id) {
    return ITEMS[static_cast<std::size_t>(id)];
  }
  const int id;
  explicit Item(int rawId, RegistrationMode mode = RegistrationMode::Immediate) : id(256 + rawId) {
    if(mode == RegistrationMode::Immediate) {
      registerInItemsArray(this);
    }
  }
  // Leaf registerClass paths that construct through Deferred bases must call this.
  static void registerInItemsArray(Item* item);
  virtual ~Item() = default;
  // --- builder-style setters (return Item* for chaining, faithful to Java) ---
  Item* setTextureId(int textureId) {
    textureId_ = textureId;
    return this;
  }
  Item* setMaxCount(int maxCount) {
    maxCount_ = maxCount;
    return this;
  }
  Item* setTexturePosition(int x, int y) {
    textureId_ = x + y * 16;
    return this;
  }
  Item* setHandheld() {
    handheld_ = true;
    return this;
  }
  Item* setTranslationKey(const std::string& key) {
    translationKey_ = "item." + key;
    return this;
  }
  Item* setCraftingReturnItem(Item* item) {
    // Java throws if maxCount > 1; preserve that invariant.
    craftingReturnItem_ = item;
    return this;
  }
  // Furnace burn time in ticks; 0 means not fuel. Collected into FuelRegistry at FuelRegistration.
  Item* setFuelTime(int burnTicks) {
    fuelTime_ = burnTicks;
    return this;
  }
  // --- queries ---
  [[nodiscard]] virtual int getTextureId(int /*damage*/) const {
    return textureId_;
  }
  [[nodiscard]] int getMaxCount() const {
    return maxCount_;
  }
  [[nodiscard]] virtual int getPlacementMetadata(int /*meta*/) const {
    return 0;
  }
  [[nodiscard]] bool hasSubtypes() const {
    return hasSubtypes_;
  }
  [[nodiscard]] int getMaxDamage() const {
    return maxDamage_;
  }
  [[nodiscard]] bool isDamageable() const {
    return maxDamage_ > 0 && !hasSubtypes_;
  }
  [[nodiscard]] virtual int getAttackDamage(Entity* /*attacked*/) const {
    return 1;
  }
  [[nodiscard]] virtual bool isSuitableFor(Block* /*block*/) const {
    return false;
  }
  [[nodiscard]] bool isHandheld() const {
    return handheld_;
  }
  [[nodiscard]] virtual bool isHandheldRod() const {
    return false;
  }
  [[nodiscard]] const std::string& getTranslationKey() const {
    return translationKey_;
  }
  [[nodiscard]] virtual std::string getTranslationKey(const ItemStack* /*stack*/) const {
    return getTranslationKey();
  }
  [[nodiscard]] std::string getTranslatedName() const;
  [[nodiscard]] Item* getCraftingReturnItem() const {
    return craftingReturnItem_;
  }
  [[nodiscard]] bool hasCraftingReturnItem() const {
    return craftingReturnItem_ != nullptr;
  }
  [[nodiscard]] virtual int getFuelTime() const {
    return fuelTime_;
  }
  [[nodiscard]] virtual int getColorMultiplier(int /*color*/) const {
    return 0xFFFFFF;
  }
  [[nodiscard]] virtual float getMiningSpeedMultiplier(ItemStack* /*stack*/, Block* /*block*/) const {
    return 1.0f;
  }
  // --- behavior hooks (overridden by subclasses; bodies in subclass headers/TUs) ---
  virtual bool useOnBlock(ItemStack* /*stack*/, PlayerEntity* /*user*/, World* /*world*/, int /*x*/, int /*y*/,
                          int /*z*/, int /*side*/) {
    return false;
  }
  virtual ItemStack* use(ItemStack* stack, World* /*world*/, PlayerEntity* /*user*/) {
    return stack;
  }
  virtual bool postHit(ItemStack* /*stack*/, LivingEntity* /*target*/, LivingEntity* /*attacker*/) {
    return false;
  }
  virtual bool postMine(ItemStack* /*stack*/, int /*blockId*/, int /*x*/, int /*y*/, int /*z*/,
                        LivingEntity* /*miner*/) {
    return false;
  }
  virtual void useOnEntity(ItemStack* /*stack*/, LivingEntity* /*entity*/) {}
  virtual void inventoryTick(ItemStack* /*stack*/, World* /*world*/, Entity* /*entity*/, int /*slot*/,
                             bool /*selected*/) {
  }
  virtual void onCraft(ItemStack* /*stack*/, World* /*world*/, PlayerEntity* /*player*/) {}

protected:
  Item* setHasSubtypes(bool v) {
    hasSubtypes_ = v;
    return this;
  }
  Item* setMaxDamage(int v) {
    maxDamage_ = v;
    return this;
  }
  int maxCount_ = 64;
  int maxDamage_ = 0;
  int textureId_ = 0;
  bool handheld_ = false;
  bool hasSubtypes_ = false;
  Item* craftingReturnItem_ = nullptr;
  int fuelTime_ = 0;
  std::string translationKey_;
};
} // namespace net::minecraft
