#include "net/minecraft/item/Item.hpp"

#include "net/minecraft/registry/VanillaRegistry.hpp"

#include "net/minecraft/client/resource/language/I18n.hpp"

namespace net::minecraft {

// Storage for Item's static members (Java: static Item[] ITEMS, static Random).
std::array<Item*, Item::ITEM_COUNT> Item::ITEMS{};
JavaRandom Item::random{};

Item* Item::IRON_SHOVEL = nullptr;
Item* Item::IRON_PICKAXE = nullptr;
Item* Item::IRON_AXE = nullptr;
Item* Item::FLINT_AND_STEEL = nullptr;
Item* Item::APPLE = nullptr;
Item* Item::BOW = nullptr;
Item* Item::ARROW = nullptr;
Item* Item::COAL = nullptr;
Item* Item::DIAMOND = nullptr;
Item* Item::IRON_INGOT = nullptr;
Item* Item::GOLD_INGOT = nullptr;
Item* Item::IRON_SWORD = nullptr;
Item* Item::WOODEN_SWORD = nullptr;
Item* Item::WOODEN_SHOVEL = nullptr;
Item* Item::WOODEN_PICKAXE = nullptr;
Item* Item::WOODEN_AXE = nullptr;
Item* Item::STONE_SWORD = nullptr;
Item* Item::STONE_SHOVEL = nullptr;
Item* Item::STONE_PICKAXE = nullptr;
Item* Item::STONE_AXE = nullptr;
Item* Item::DIAMOND_SWORD = nullptr;
Item* Item::DIAMOND_SHOVEL = nullptr;
Item* Item::DIAMOND_PICKAXE = nullptr;
Item* Item::DIAMOND_AXE = nullptr;
Item* Item::STICK = nullptr;
Item* Item::BOWL = nullptr;
Item* Item::MUSHROOM_STEW = nullptr;
Item* Item::GOLDEN_SWORD = nullptr;
Item* Item::GOLDEN_SHOVEL = nullptr;
Item* Item::GOLDEN_PICKAXE = nullptr;
Item* Item::GOLDEN_AXE = nullptr;
Item* Item::STRING = nullptr;
Item* Item::FEATHER = nullptr;
Item* Item::GUNPOWDER = nullptr;
Item* Item::WOODEN_HOE = nullptr;
Item* Item::STONE_HOE = nullptr;
Item* Item::IRON_HOE = nullptr;
Item* Item::DIAMOND_HOE = nullptr;
Item* Item::GOLDEN_HOE = nullptr;
Item* Item::SEEDS = nullptr;
Item* Item::WHEAT = nullptr;
Item* Item::BREAD = nullptr;
Item* Item::LEATHER_HELMET = nullptr;
Item* Item::LEATHER_CHESTPLATE = nullptr;
Item* Item::LEATHER_LEGGINGS = nullptr;
Item* Item::LEATHER_BOOTS = nullptr;
Item* Item::CHAIN_HELMET = nullptr;
Item* Item::CHAIN_CHESTPLATE = nullptr;
Item* Item::CHAIN_LEGGINGS = nullptr;
Item* Item::CHAIN_BOOTS = nullptr;
Item* Item::IRON_HELMET = nullptr;
Item* Item::IRON_CHESTPLATE = nullptr;
Item* Item::IRON_LEGGINGS = nullptr;
Item* Item::IRON_BOOTS = nullptr;
Item* Item::DIAMOND_HELMET = nullptr;
Item* Item::DIAMOND_CHESTPLATE = nullptr;
Item* Item::DIAMOND_LEGGINGS = nullptr;
Item* Item::DIAMOND_BOOTS = nullptr;
Item* Item::GOLDEN_HELMET = nullptr;
Item* Item::GOLDEN_CHESTPLATE = nullptr;
Item* Item::GOLDEN_LEGGINGS = nullptr;
Item* Item::GOLDEN_BOOTS = nullptr;
Item* Item::FLINT = nullptr;
Item* Item::RAW_PORKCHOP = nullptr;
Item* Item::COOKED_PORKCHOP = nullptr;
Item* Item::PAINTING = nullptr;
Item* Item::GOLDEN_APPLE = nullptr;
Item* Item::SIGN = nullptr;
Item* Item::WOODEN_DOOR = nullptr;
Item* Item::BUCKET = nullptr;
Item* Item::WATER_BUCKET = nullptr;
Item* Item::LAVA_BUCKET = nullptr;
Item* Item::MINECART = nullptr;
Item* Item::SADDLE = nullptr;
Item* Item::IRON_DOOR = nullptr;
Item* Item::REDSTONE = nullptr;
Item* Item::SNOWBALL = nullptr;
Item* Item::BOAT = nullptr;
Item* Item::LEATHER = nullptr;
Item* Item::MILK_BUCKET = nullptr;
Item* Item::BRICK = nullptr;
Item* Item::CLAY = nullptr;
Item* Item::SUGAR_CANE = nullptr;
Item* Item::PAPER = nullptr;
Item* Item::BOOK = nullptr;
Item* Item::SLIMEBALL = nullptr;
Item* Item::CHEST_MINECART = nullptr;
Item* Item::FURNACE_MINECART = nullptr;
Item* Item::EGG = nullptr;
Item* Item::COMPASS = nullptr;
Item* Item::FISHING_ROD = nullptr;
Item* Item::CLOCK = nullptr;
Item* Item::GLOWSTONE_DUST = nullptr;
Item* Item::RAW_FISH = nullptr;
Item* Item::COOKED_FISH = nullptr;
Item* Item::DYE = nullptr;
Item* Item::BONE = nullptr;
Item* Item::SUGAR = nullptr;
Item* Item::CAKE = nullptr;
Item* Item::BED = nullptr;
Item* Item::REPEATER = nullptr;
Item* Item::COOKIE = nullptr;
Item* Item::MAP = nullptr;
Item* Item::SHEARS = nullptr;
Item* Item::RECORD_THIRTEEN = nullptr;
Item* Item::RECORD_CAT = nullptr;

std::string Item::getTranslatedName() const
{
    return client::resource::language::I18n::getTranslation(getTranslationKey() + ".name");
}

void initializeItems()
{
    registry::runVanillaBootstrap();
}

} // namespace net::minecraft
