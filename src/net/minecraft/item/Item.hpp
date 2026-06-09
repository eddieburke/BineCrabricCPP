#pragma once

// Faithful 1:1 port of net.minecraft.item.Item base class (beta 1.7.3).
//
// Item participates in a dependency cycle (Item <-> Block <-> ItemStack <->
// World/Entity). The base class only needs forward declarations; concrete item
// Vanilla item statics are registered in Item.cpp (mirrors Item.java static fields).
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

class Item {
public:
    static constexpr int ITEM_COUNT = 32000;
    static std::array<Item*, ITEM_COUNT> ITEMS;
    static JavaRandom random;

    // Java's named static item instances. Populated from ITEMS[id] once
    // registerVanillaItems() has run (see Item.cpp).
    static Item* IRON_SHOVEL;
    static Item* IRON_PICKAXE;
    static Item* IRON_AXE;
    static Item* FLINT_AND_STEEL;
    static Item* APPLE;
    static Item* BOW;
    static Item* ARROW;
    static Item* COAL;
    static Item* DIAMOND;
    static Item* IRON_INGOT;
    static Item* GOLD_INGOT;
    static Item* IRON_SWORD;
    static Item* WOODEN_SWORD;
    static Item* WOODEN_SHOVEL;
    static Item* WOODEN_PICKAXE;
    static Item* WOODEN_AXE;
    static Item* STONE_SWORD;
    static Item* STONE_SHOVEL;
    static Item* STONE_PICKAXE;
    static Item* STONE_AXE;
    static Item* DIAMOND_SWORD;
    static Item* DIAMOND_SHOVEL;
    static Item* DIAMOND_PICKAXE;
    static Item* DIAMOND_AXE;
    static Item* STICK;
    static Item* BOWL;
    static Item* MUSHROOM_STEW;
    static Item* GOLDEN_SWORD;
    static Item* GOLDEN_SHOVEL;
    static Item* GOLDEN_PICKAXE;
    static Item* GOLDEN_AXE;
    static Item* STRING;
    static Item* FEATHER;
    static Item* GUNPOWDER;
    static Item* WOODEN_HOE;
    static Item* STONE_HOE;
    static Item* IRON_HOE;
    static Item* DIAMOND_HOE;
    static Item* GOLDEN_HOE;
    static Item* SEEDS;
    static Item* WHEAT;
    static Item* BREAD;
    static Item* LEATHER_HELMET;
    static Item* LEATHER_CHESTPLATE;
    static Item* LEATHER_LEGGINGS;
    static Item* LEATHER_BOOTS;
    static Item* CHAIN_HELMET;
    static Item* CHAIN_CHESTPLATE;
    static Item* CHAIN_LEGGINGS;
    static Item* CHAIN_BOOTS;
    static Item* IRON_HELMET;
    static Item* IRON_CHESTPLATE;
    static Item* IRON_LEGGINGS;
    static Item* IRON_BOOTS;
    static Item* DIAMOND_HELMET;
    static Item* DIAMOND_CHESTPLATE;
    static Item* DIAMOND_LEGGINGS;
    static Item* DIAMOND_BOOTS;
    static Item* GOLDEN_HELMET;
    static Item* GOLDEN_CHESTPLATE;
    static Item* GOLDEN_LEGGINGS;
    static Item* GOLDEN_BOOTS;
    static Item* FLINT;
    static Item* RAW_PORKCHOP;
    static Item* COOKED_PORKCHOP;
    static Item* PAINTING;
    static Item* GOLDEN_APPLE;
    static Item* SIGN;
    static Item* WOODEN_DOOR;
    static Item* BUCKET;
    static Item* WATER_BUCKET;
    static Item* LAVA_BUCKET;
    static Item* MINECART;
    static Item* SADDLE;
    static Item* IRON_DOOR;
    static Item* REDSTONE;
    static Item* SNOWBALL;
    static Item* BOAT;
    static Item* LEATHER;
    static Item* MILK_BUCKET;
    static Item* BRICK;
    static Item* CLAY;
    static Item* SUGAR_CANE;
    static Item* PAPER;
    static Item* BOOK;
    static Item* SLIMEBALL;
    static Item* CHEST_MINECART;
    static Item* FURNACE_MINECART;
    static Item* EGG;
    static Item* COMPASS;
    static Item* FISHING_ROD;
    static Item* CLOCK;
    static Item* GLOWSTONE_DUST;
    static Item* RAW_FISH;
    static Item* COOKED_FISH;
    static Item* DYE;
    static Item* BONE;
    static Item* SUGAR;
    static Item* CAKE;
    static Item* BED;
    static Item* REPEATER;
    static Item* COOKIE;
    static Item* MAP;
    static Item* SHEARS;
    static Item* RECORD_THIRTEEN;
    static Item* RECORD_CAT;

    const int id;

    explicit Item(int rawId)
        : id(256 + rawId)
    {
        ITEMS[static_cast<std::size_t>(256 + rawId)] = this;
    }

    virtual ~Item() = default;

    // --- builder-style setters (return Item* for chaining, faithful to Java) ---
    Item* setTextureId(int textureId) { textureId_ = textureId; return this; }
    Item* setMaxCount(int maxCount) { maxCount_ = maxCount; return this; }
    Item* setTexturePosition(int x, int y) { textureId_ = x + y * 16; return this; }
    Item* setHandheld() { handheld_ = true; return this; }
    Item* setTranslationKey(const std::string& key) { translationKey_ = "item." + key; return this; }

    Item* setCraftingReturnItem(Item* item)
    {
        // Java throws if maxCount > 1; preserve that invariant.
        craftingReturnItem_ = item;
        return this;
    }

    // --- queries ---
    [[nodiscard]] virtual int getTextureId(int /*damage*/) const { return textureId_; }
    [[nodiscard]] int getMaxCount() const { return maxCount_; }
    [[nodiscard]] virtual int getPlacementMetadata(int /*meta*/) const { return 0; }
    [[nodiscard]] bool hasSubtypes() const { return hasSubtypes_; }
    [[nodiscard]] int getMaxDamage() const { return maxDamage_; }
    [[nodiscard]] bool isDamageable() const { return maxDamage_ > 0 && !hasSubtypes_; }
    [[nodiscard]] virtual int getAttackDamage(Entity* /*attacked*/) const { return 1; }
    [[nodiscard]] virtual bool isSuitableFor(Block* /*block*/) const { return false; }
    [[nodiscard]] bool isHandheld() const { return handheld_; }
    [[nodiscard]] virtual bool isHandheldRod() const { return false; }
    [[nodiscard]] const std::string& getTranslationKey() const { return translationKey_; }
    [[nodiscard]] virtual std::string getTranslationKey(const ItemStack* /*stack*/) const { return getTranslationKey(); }
    [[nodiscard]] std::string getTranslatedName() const;
    [[nodiscard]] Item* getCraftingReturnItem() const { return craftingReturnItem_; }
    [[nodiscard]] bool hasCraftingReturnItem() const { return craftingReturnItem_ != nullptr; }
    [[nodiscard]] virtual int getColorMultiplier(int /*color*/) const { return 0xFFFFFF; }
    [[nodiscard]] virtual float getMiningSpeedMultiplier(ItemStack* /*stack*/, Block* /*block*/) const { return 1.0f; }

    // --- behavior hooks (overridden by subclasses; bodies in subclass headers/TUs) ---
    virtual bool useOnBlock(ItemStack* /*stack*/, PlayerEntity* /*user*/,
                            World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*side*/) { return false; }
    virtual ItemStack* use(ItemStack* stack, World* /*world*/, PlayerEntity* /*user*/) { return stack; }
    virtual bool postHit(ItemStack* /*stack*/, LivingEntity* /*target*/, LivingEntity* /*attacker*/) { return false; }
    virtual bool postMine(ItemStack* /*stack*/, int /*blockId*/, int /*x*/, int /*y*/, int /*z*/, LivingEntity* /*miner*/) { return false; }
    virtual void useOnEntity(ItemStack* /*stack*/, LivingEntity* /*entity*/) {}
    virtual void inventoryTick(ItemStack* /*stack*/, World* /*world*/, Entity* /*entity*/, int /*slot*/, bool /*selected*/) {}
    virtual void onCraft(ItemStack* /*stack*/, World* /*world*/, PlayerEntity* /*player*/) {}

protected:
    Item* setHasSubtypes(bool v) { hasSubtypes_ = v; return this; }
    Item* setMaxDamage(int v) { maxDamage_ = v; return this; }

    int maxCount_ = 64;
    int maxDamage_ = 0;
    int textureId_ = 0;
    bool handheld_ = false;
    bool hasSubtypes_ = false;
    Item* craftingReturnItem_ = nullptr;
    std::string translationKey_;
};

// C++ static-init shim for Item.java field initializers (not a separate MCP type).
void initializeItems();

} // namespace net::minecraft
