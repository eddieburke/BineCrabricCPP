#include "net/minecraft/item/Item.hpp"

#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/BedItem.hpp"
#include "net/minecraft/item/BoatItem.hpp"
#include "net/minecraft/item/BowItem.hpp"
#include "net/minecraft/item/BucketItem.hpp"
#include "net/minecraft/item/CoalItem.hpp"
#include "net/minecraft/item/DoorItem.hpp"
#include "net/minecraft/item/DyeItem.hpp"
#include "net/minecraft/item/EggItem.hpp"
#include "net/minecraft/item/FishingRodItem.hpp"
#include "net/minecraft/item/FlintAndSteel.hpp"
#include "net/minecraft/item/FoodItem.hpp"
#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/MinecartItem.hpp"
#include "net/minecraft/item/MushroomStewItem.hpp"
#include "net/minecraft/item/MusicDiscItem.hpp"
#include "net/minecraft/item/PaintingItem.hpp"
#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/RedstoneItem.hpp"
#include "net/minecraft/item/SaddleItem.hpp"
#include "net/minecraft/item/SecondaryBlockItem.hpp"
#include "net/minecraft/item/SeedsItem.hpp"
#include "net/minecraft/item/ShearsItem.hpp"
#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/SignItem.hpp"
#include "net/minecraft/item/SnowballItem.hpp"
#include "net/minecraft/item/StackableFoodItem.hpp"
#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

#include "net/minecraft/client/resource/language/I18n.hpp"

#include <mutex>

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

namespace {

std::once_flag g_itemsInit;

// Mirrors Item.java static field initializers (beta 1.7.3 MCP).
void registerVanillaItems()
{
    using item::ArmorItem;
    using item::AxeItem;
    using item::BedItem;
    using item::BoatItem;
    using item::BowItem;
    using item::BucketItem;
    using item::CoalItem;
    using item::DoorItem;
    using item::DyeItem;
    using item::EggItem;
    using item::FishingRodItem;
    using item::FlintAndSteel;
    using item::FoodItem;
    using item::HoeItem;
    using item::MapItem;
    using item::MinecartItem;
    using item::MushroomStewItem;
    using item::MusicDiscItem;
    using item::PaintingItem;
    using item::PickaxeItem;
    using item::RedstoneItem;
    using item::SaddleItem;
    using item::SecondaryBlockItem;
    using item::SeedsItem;
    using item::ShearsItem;
    using item::ShovelItem;
    using item::SignItem;
    using item::SnowballItem;
    using item::StackableFoodItem;
    using item::SwordItem;
    using item::ToolMaterial;

    static ShovelItem IRON_SHOVEL(0, ToolMaterial::Iron);
    IRON_SHOVEL.setTexturePosition(2, 5)->setTranslationKey("shovelIron");

    static PickaxeItem IRON_PICKAXE(1, ToolMaterial::Iron);
    IRON_PICKAXE.setTexturePosition(2, 6)->setTranslationKey("pickaxeIron");

    static AxeItem IRON_AXE(2, ToolMaterial::Iron);
    IRON_AXE.setTexturePosition(2, 7)->setTranslationKey("hatchetIron");

    static FlintAndSteel FLINT_AND_STEEL(3);
    FLINT_AND_STEEL.setTexturePosition(5, 0)->setTranslationKey("flintAndSteel");

    static FoodItem APPLE(4, 4, false);
    APPLE.setTexturePosition(10, 0)->setTranslationKey("apple");

    static BowItem BOW(5);
    BOW.setTexturePosition(5, 1)->setTranslationKey("bow");

    static Item ARROW(6);
    ARROW.setTexturePosition(5, 2)->setTranslationKey("arrow");

    static CoalItem COAL(7);
    COAL.setTexturePosition(7, 0)->setTranslationKey("coal");

    static Item DIAMOND(8);
    DIAMOND.setTexturePosition(7, 3)->setTranslationKey("emerald");

    static Item IRON_INGOT(9);
    IRON_INGOT.setTexturePosition(7, 1)->setTranslationKey("ingotIron");

    static Item GOLD_INGOT(10);
    GOLD_INGOT.setTexturePosition(7, 2)->setTranslationKey("ingotGold");

    static SwordItem IRON_SWORD(11, ToolMaterial::Iron);
    IRON_SWORD.setTexturePosition(2, 4)->setTranslationKey("swordIron");

    static SwordItem WOODEN_SWORD(12, ToolMaterial::Wood);
    WOODEN_SWORD.setTexturePosition(0, 4)->setTranslationKey("swordWood");

    static ShovelItem WOODEN_SHOVEL(13, ToolMaterial::Wood);
    WOODEN_SHOVEL.setTexturePosition(0, 5)->setTranslationKey("shovelWood");

    static PickaxeItem WOODEN_PICKAXE(14, ToolMaterial::Wood);
    WOODEN_PICKAXE.setTexturePosition(0, 6)->setTranslationKey("pickaxeWood");

    static AxeItem WOODEN_AXE(15, ToolMaterial::Wood);
    WOODEN_AXE.setTexturePosition(0, 7)->setTranslationKey("hatchetWood");

    static SwordItem STONE_SWORD(16, ToolMaterial::Stone);
    STONE_SWORD.setTexturePosition(1, 4)->setTranslationKey("swordStone");

    static ShovelItem STONE_SHOVEL(17, ToolMaterial::Stone);
    STONE_SHOVEL.setTexturePosition(1, 5)->setTranslationKey("shovelStone");

    static PickaxeItem STONE_PICKAXE(18, ToolMaterial::Stone);
    STONE_PICKAXE.setTexturePosition(1, 6)->setTranslationKey("pickaxeStone");

    static AxeItem STONE_AXE(19, ToolMaterial::Stone);
    STONE_AXE.setTexturePosition(1, 7)->setTranslationKey("hatchetStone");

    static SwordItem DIAMOND_SWORD(20, ToolMaterial::Diamond);
    DIAMOND_SWORD.setTexturePosition(3, 4)->setTranslationKey("swordDiamond");

    static ShovelItem DIAMOND_SHOVEL(21, ToolMaterial::Diamond);
    DIAMOND_SHOVEL.setTexturePosition(3, 5)->setTranslationKey("shovelDiamond");

    static PickaxeItem DIAMOND_PICKAXE(22, ToolMaterial::Diamond);
    DIAMOND_PICKAXE.setTexturePosition(3, 6)->setTranslationKey("pickaxeDiamond");

    static AxeItem DIAMOND_AXE(23, ToolMaterial::Diamond);
    DIAMOND_AXE.setTexturePosition(3, 7)->setTranslationKey("hatchetDiamond");

    static Item STICK(24);
    STICK.setTexturePosition(5, 3)->setHandheld()->setTranslationKey("stick");

    static Item BOWL(25);
    BOWL.setTexturePosition(7, 4)->setTranslationKey("bowl");

    static MushroomStewItem MUSHROOM_STEW(26, 10);
    MUSHROOM_STEW.setTexturePosition(8, 4)->setTranslationKey("mushroomStew");

    static SwordItem GOLDEN_SWORD(27, ToolMaterial::Gold);
    GOLDEN_SWORD.setTexturePosition(4, 4)->setTranslationKey("swordGold");

    static ShovelItem GOLDEN_SHOVEL(28, ToolMaterial::Gold);
    GOLDEN_SHOVEL.setTexturePosition(4, 5)->setTranslationKey("shovelGold");

    static PickaxeItem GOLDEN_PICKAXE(29, ToolMaterial::Gold);
    GOLDEN_PICKAXE.setTexturePosition(4, 6)->setTranslationKey("pickaxeGold");

    static AxeItem GOLDEN_AXE(30, ToolMaterial::Gold);
    GOLDEN_AXE.setTexturePosition(4, 7)->setTranslationKey("hatchetGold");

    static Item STRING(31);
    STRING.setTexturePosition(8, 0)->setTranslationKey("string");

    static Item FEATHER(32);
    FEATHER.setTexturePosition(8, 1)->setTranslationKey("feather");

    static Item GUNPOWDER(33);
    GUNPOWDER.setTexturePosition(8, 2)->setTranslationKey("sulphur");

    static HoeItem WOODEN_HOE(34, ToolMaterial::Wood);
    WOODEN_HOE.setTexturePosition(0, 8)->setTranslationKey("hoeWood");

    static HoeItem STONE_HOE(35, ToolMaterial::Stone);
    STONE_HOE.setTexturePosition(1, 8)->setTranslationKey("hoeStone");

    static HoeItem IRON_HOE(36, ToolMaterial::Iron);
    IRON_HOE.setTexturePosition(2, 8)->setTranslationKey("hoeIron");

    static HoeItem DIAMOND_HOE(37, ToolMaterial::Diamond);
    DIAMOND_HOE.setTexturePosition(3, 8)->setTranslationKey("hoeDiamond");

    static HoeItem GOLDEN_HOE(38, ToolMaterial::Gold);
    GOLDEN_HOE.setTexturePosition(4, 8)->setTranslationKey("hoeGold");

    static SeedsItem SEEDS(39, 59);
    SEEDS.setTexturePosition(9, 0)->setTranslationKey("seeds");

    static Item WHEAT(40);
    WHEAT.setTexturePosition(9, 1)->setTranslationKey("wheat");

    static FoodItem BREAD(41, 5, false);
    BREAD.setTexturePosition(9, 2)->setTranslationKey("bread");

    static ArmorItem LEATHER_HELMET(42, 0, 0, 0);
    LEATHER_HELMET.setTexturePosition(0, 0)->setTranslationKey("helmetCloth");
    static ArmorItem LEATHER_CHESTPLATE(43, 0, 0, 1);
    LEATHER_CHESTPLATE.setTexturePosition(0, 1)->setTranslationKey("chestplateCloth");
    static ArmorItem LEATHER_LEGGINGS(44, 0, 0, 2);
    LEATHER_LEGGINGS.setTexturePosition(0, 2)->setTranslationKey("leggingsCloth");
    static ArmorItem LEATHER_BOOTS(45, 0, 0, 3);
    LEATHER_BOOTS.setTexturePosition(0, 3)->setTranslationKey("bootsCloth");

    static ArmorItem CHAIN_HELMET(46, 1, 1, 0);
    CHAIN_HELMET.setTexturePosition(1, 0)->setTranslationKey("helmetChain");
    static ArmorItem CHAIN_CHESTPLATE(47, 1, 1, 1);
    CHAIN_CHESTPLATE.setTexturePosition(1, 1)->setTranslationKey("chestplateChain");
    static ArmorItem CHAIN_LEGGINGS(48, 1, 1, 2);
    CHAIN_LEGGINGS.setTexturePosition(1, 2)->setTranslationKey("leggingsChain");
    static ArmorItem CHAIN_BOOTS(49, 1, 1, 3);
    CHAIN_BOOTS.setTexturePosition(1, 3)->setTranslationKey("bootsChain");

    static ArmorItem IRON_HELMET(50, 2, 2, 0);
    IRON_HELMET.setTexturePosition(2, 0)->setTranslationKey("helmetIron");
    static ArmorItem IRON_CHESTPLATE(51, 2, 2, 1);
    IRON_CHESTPLATE.setTexturePosition(2, 1)->setTranslationKey("chestplateIron");
    static ArmorItem IRON_LEGGINGS(52, 2, 2, 2);
    IRON_LEGGINGS.setTexturePosition(2, 2)->setTranslationKey("leggingsIron");
    static ArmorItem IRON_BOOTS(53, 2, 2, 3);
    IRON_BOOTS.setTexturePosition(2, 3)->setTranslationKey("bootsIron");

    static ArmorItem DIAMOND_HELMET(54, 3, 3, 0);
    DIAMOND_HELMET.setTexturePosition(3, 0)->setTranslationKey("helmetDiamond");
    static ArmorItem DIAMOND_CHESTPLATE(55, 3, 3, 1);
    DIAMOND_CHESTPLATE.setTexturePosition(3, 1)->setTranslationKey("chestplateDiamond");
    static ArmorItem DIAMOND_LEGGINGS(56, 3, 3, 2);
    DIAMOND_LEGGINGS.setTexturePosition(3, 2)->setTranslationKey("leggingsDiamond");
    static ArmorItem DIAMOND_BOOTS(57, 3, 3, 3);
    DIAMOND_BOOTS.setTexturePosition(3, 3)->setTranslationKey("bootsDiamond");

    static ArmorItem GOLDEN_HELMET(58, 1, 4, 0);
    GOLDEN_HELMET.setTexturePosition(4, 0)->setTranslationKey("helmetGold");
    static ArmorItem GOLDEN_CHESTPLATE(59, 1, 4, 1);
    GOLDEN_CHESTPLATE.setTexturePosition(4, 1)->setTranslationKey("chestplateGold");
    static ArmorItem GOLDEN_LEGGINGS(60, 1, 4, 2);
    GOLDEN_LEGGINGS.setTexturePosition(4, 2)->setTranslationKey("leggingsGold");
    static ArmorItem GOLDEN_BOOTS(61, 1, 4, 3);
    GOLDEN_BOOTS.setTexturePosition(4, 3)->setTranslationKey("bootsGold");

    static Item FLINT(62);
    FLINT.setTexturePosition(6, 0)->setTranslationKey("flint");

    static FoodItem RAW_PORKCHOP(63, 3, true);
    RAW_PORKCHOP.setTexturePosition(7, 5)->setTranslationKey("porkchopRaw");

    static FoodItem COOKED_PORKCHOP(64, 8, true);
    COOKED_PORKCHOP.setTexturePosition(8, 5)->setTranslationKey("porkchopCooked");

    static PaintingItem PAINTING(65);
    PAINTING.setTexturePosition(10, 1)->setTranslationKey("painting");

    static FoodItem GOLDEN_APPLE(66, 42, false);
    GOLDEN_APPLE.setTexturePosition(11, 0)->setTranslationKey("appleGold");

    static SignItem SIGN(67);
    SIGN.setTexturePosition(10, 2)->setTranslationKey("sign");

    static DoorItem WOODEN_DOOR(68, block::material::Material::WOOD);
    WOODEN_DOOR.setTexturePosition(11, 2)->setTranslationKey("doorWood");

    static BucketItem BUCKET(69, 0);
    BUCKET.setTexturePosition(10, 4)->setTranslationKey("bucket");

    static BucketItem WATER_BUCKET(70, Block::FLOWING_WATER != nullptr ? Block::FLOWING_WATER->id : 8);
    WATER_BUCKET.setTexturePosition(11, 4)->setTranslationKey("bucketWater")->setCraftingReturnItem(&BUCKET);

    static BucketItem LAVA_BUCKET(71, Block::FLOWING_LAVA != nullptr ? Block::FLOWING_LAVA->id : 10);
    LAVA_BUCKET.setTexturePosition(12, 4)->setTranslationKey("bucketLava")->setCraftingReturnItem(&BUCKET);

    static MinecartItem MINECART(72, 0);
    MINECART.setTexturePosition(7, 8)->setTranslationKey("minecart");

    static SaddleItem SADDLE(73);
    SADDLE.setTexturePosition(8, 6)->setTranslationKey("saddle");

    static DoorItem IRON_DOOR(74, block::material::Material::METAL);
    IRON_DOOR.setTexturePosition(12, 2)->setTranslationKey("doorIron");

    static RedstoneItem REDSTONE(75);
    REDSTONE.setTexturePosition(8, 3)->setTranslationKey("redstone");

    static SnowballItem SNOWBALL(76);
    SNOWBALL.setTexturePosition(14, 0)->setTranslationKey("snowball");

    static BoatItem BOAT(77);
    BOAT.setTexturePosition(8, 8)->setTranslationKey("boat");

    static Item LEATHER(78);
    LEATHER.setTexturePosition(7, 6)->setTranslationKey("leather");

    static BucketItem MILK_BUCKET(79, -1);
    MILK_BUCKET.setTexturePosition(13, 4)->setTranslationKey("milk")->setCraftingReturnItem(&BUCKET);

    static Item BRICK(80);
    BRICK.setTexturePosition(6, 1)->setTranslationKey("brick");

    static Item CLAY(81);
    CLAY.setTexturePosition(9, 3)->setTranslationKey("clay");

    static SecondaryBlockItem SUGAR_CANE(82, Block::SUGAR_CANE);
    SUGAR_CANE.setTexturePosition(11, 1)->setTranslationKey("reeds");

    static Item PAPER(83);
    PAPER.setTexturePosition(10, 3)->setTranslationKey("paper");

    static Item BOOK(84);
    BOOK.setTexturePosition(11, 3)->setTranslationKey("book");

    static Item SLIMEBALL(85);
    SLIMEBALL.setTexturePosition(14, 1)->setTranslationKey("slimeball");

    static MinecartItem CHEST_MINECART(86, 1);
    CHEST_MINECART.setTexturePosition(7, 9)->setTranslationKey("minecartChest");

    static MinecartItem FURNACE_MINECART(87, 2);
    FURNACE_MINECART.setTexturePosition(7, 10)->setTranslationKey("minecartFurnace");

    static EggItem EGG(88);
    EGG.setTexturePosition(12, 0)->setTranslationKey("egg");

    static Item COMPASS(89);
    COMPASS.setTexturePosition(6, 3)->setTranslationKey("compass");

    static FishingRodItem FISHING_ROD(90);
    FISHING_ROD.setTexturePosition(5, 4)->setTranslationKey("fishingRod");

    static Item CLOCK(91);
    CLOCK.setTexturePosition(6, 4)->setTranslationKey("clock");

    static Item GLOWSTONE_DUST(92);
    GLOWSTONE_DUST.setTexturePosition(9, 4)->setTranslationKey("yellowDust");

    static FoodItem RAW_FISH(93, 2, false);
    RAW_FISH.setTexturePosition(9, 5)->setTranslationKey("fishRaw");

    static FoodItem COOKED_FISH(94, 5, false);
    COOKED_FISH.setTexturePosition(10, 5)->setTranslationKey("fishCooked");

    static DyeItem DYE(95);
    DYE.setTexturePosition(14, 4)->setTranslationKey("dyePowder");

    static Item BONE(96);
    BONE.setTexturePosition(12, 1)->setHandheld()->setTranslationKey("bone");

    static Item SUGAR(97);
    SUGAR.setTexturePosition(13, 0)->setHandheld()->setTranslationKey("sugar");

    static SecondaryBlockItem CAKE(98, Block::CAKE);
    CAKE.setMaxCount(1)->setTexturePosition(13, 1)->setTranslationKey("cake");

    static BedItem BED(99);
    BED.setMaxCount(1)->setTexturePosition(13, 2)->setTranslationKey("bed");

    static SecondaryBlockItem REPEATER(100, Block::REPEATER);
    REPEATER.setTexturePosition(6, 5)->setTranslationKey("diode");

    static StackableFoodItem COOKIE(101, 1, false, 8);
    COOKIE.setTexturePosition(12, 5)->setTranslationKey("cookie");

    static MapItem MAP(102);
    MAP.setTexturePosition(12, 3)->setTranslationKey("map");

    static ShearsItem SHEARS(103);
    SHEARS.setTexturePosition(13, 5)->setTranslationKey("shears");

    static MusicDiscItem RECORD_THIRTEEN(2000, "13");
    RECORD_THIRTEEN.setTexturePosition(0, 15)->setTranslationKey("record");

    static MusicDiscItem RECORD_CAT(2001, "cat");
    RECORD_CAT.setTexturePosition(1, 15)->setTranslationKey("record");

    // Bind Java's named static item instances to their registered objects.
    Item::IRON_SHOVEL = Item::ITEMS[256];
    Item::IRON_PICKAXE = Item::ITEMS[257];
    Item::IRON_AXE = Item::ITEMS[258];
    Item::FLINT_AND_STEEL = Item::ITEMS[259];
    Item::APPLE = Item::ITEMS[260];
    Item::BOW = Item::ITEMS[261];
    Item::ARROW = Item::ITEMS[262];
    Item::COAL = Item::ITEMS[263];
    Item::DIAMOND = Item::ITEMS[264];
    Item::IRON_INGOT = Item::ITEMS[265];
    Item::GOLD_INGOT = Item::ITEMS[266];
    Item::IRON_SWORD = Item::ITEMS[267];
    Item::WOODEN_SWORD = Item::ITEMS[268];
    Item::WOODEN_SHOVEL = Item::ITEMS[269];
    Item::WOODEN_PICKAXE = Item::ITEMS[270];
    Item::WOODEN_AXE = Item::ITEMS[271];
    Item::STONE_SWORD = Item::ITEMS[272];
    Item::STONE_SHOVEL = Item::ITEMS[273];
    Item::STONE_PICKAXE = Item::ITEMS[274];
    Item::STONE_AXE = Item::ITEMS[275];
    Item::DIAMOND_SWORD = Item::ITEMS[276];
    Item::DIAMOND_SHOVEL = Item::ITEMS[277];
    Item::DIAMOND_PICKAXE = Item::ITEMS[278];
    Item::DIAMOND_AXE = Item::ITEMS[279];
    Item::STICK = Item::ITEMS[280];
    Item::BOWL = Item::ITEMS[281];
    Item::MUSHROOM_STEW = Item::ITEMS[282];
    Item::GOLDEN_SWORD = Item::ITEMS[283];
    Item::GOLDEN_SHOVEL = Item::ITEMS[284];
    Item::GOLDEN_PICKAXE = Item::ITEMS[285];
    Item::GOLDEN_AXE = Item::ITEMS[286];
    Item::STRING = Item::ITEMS[287];
    Item::FEATHER = Item::ITEMS[288];
    Item::GUNPOWDER = Item::ITEMS[289];
    Item::WOODEN_HOE = Item::ITEMS[290];
    Item::STONE_HOE = Item::ITEMS[291];
    Item::IRON_HOE = Item::ITEMS[292];
    Item::DIAMOND_HOE = Item::ITEMS[293];
    Item::GOLDEN_HOE = Item::ITEMS[294];
    Item::SEEDS = Item::ITEMS[295];
    Item::WHEAT = Item::ITEMS[296];
    Item::BREAD = Item::ITEMS[297];
    Item::LEATHER_HELMET = Item::ITEMS[298];
    Item::LEATHER_CHESTPLATE = Item::ITEMS[299];
    Item::LEATHER_LEGGINGS = Item::ITEMS[300];
    Item::LEATHER_BOOTS = Item::ITEMS[301];
    Item::CHAIN_HELMET = Item::ITEMS[302];
    Item::CHAIN_CHESTPLATE = Item::ITEMS[303];
    Item::CHAIN_LEGGINGS = Item::ITEMS[304];
    Item::CHAIN_BOOTS = Item::ITEMS[305];
    Item::IRON_HELMET = Item::ITEMS[306];
    Item::IRON_CHESTPLATE = Item::ITEMS[307];
    Item::IRON_LEGGINGS = Item::ITEMS[308];
    Item::IRON_BOOTS = Item::ITEMS[309];
    Item::DIAMOND_HELMET = Item::ITEMS[310];
    Item::DIAMOND_CHESTPLATE = Item::ITEMS[311];
    Item::DIAMOND_LEGGINGS = Item::ITEMS[312];
    Item::DIAMOND_BOOTS = Item::ITEMS[313];
    Item::GOLDEN_HELMET = Item::ITEMS[314];
    Item::GOLDEN_CHESTPLATE = Item::ITEMS[315];
    Item::GOLDEN_LEGGINGS = Item::ITEMS[316];
    Item::GOLDEN_BOOTS = Item::ITEMS[317];
    Item::FLINT = Item::ITEMS[318];
    Item::RAW_PORKCHOP = Item::ITEMS[319];
    Item::COOKED_PORKCHOP = Item::ITEMS[320];
    Item::PAINTING = Item::ITEMS[321];
    Item::GOLDEN_APPLE = Item::ITEMS[322];
    Item::SIGN = Item::ITEMS[323];
    Item::WOODEN_DOOR = Item::ITEMS[324];
    Item::BUCKET = Item::ITEMS[325];
    Item::WATER_BUCKET = Item::ITEMS[326];
    Item::LAVA_BUCKET = Item::ITEMS[327];
    Item::MINECART = Item::ITEMS[328];
    Item::SADDLE = Item::ITEMS[329];
    Item::IRON_DOOR = Item::ITEMS[330];
    Item::REDSTONE = Item::ITEMS[331];
    Item::SNOWBALL = Item::ITEMS[332];
    Item::BOAT = Item::ITEMS[333];
    Item::LEATHER = Item::ITEMS[334];
    Item::MILK_BUCKET = Item::ITEMS[335];
    Item::BRICK = Item::ITEMS[336];
    Item::CLAY = Item::ITEMS[337];
    Item::SUGAR_CANE = Item::ITEMS[338];
    Item::PAPER = Item::ITEMS[339];
    Item::BOOK = Item::ITEMS[340];
    Item::SLIMEBALL = Item::ITEMS[341];
    Item::CHEST_MINECART = Item::ITEMS[342];
    Item::FURNACE_MINECART = Item::ITEMS[343];
    Item::EGG = Item::ITEMS[344];
    Item::COMPASS = Item::ITEMS[345];
    Item::FISHING_ROD = Item::ITEMS[346];
    Item::CLOCK = Item::ITEMS[347];
    Item::GLOWSTONE_DUST = Item::ITEMS[348];
    Item::RAW_FISH = Item::ITEMS[349];
    Item::COOKED_FISH = Item::ITEMS[350];
    Item::DYE = Item::ITEMS[351];
    Item::BONE = Item::ITEMS[352];
    Item::SUGAR = Item::ITEMS[353];
    Item::CAKE = Item::ITEMS[354];
    Item::BED = Item::ITEMS[355];
    Item::REPEATER = Item::ITEMS[356];
    Item::COOKIE = Item::ITEMS[357];
    Item::MAP = Item::ITEMS[358];
    Item::SHEARS = Item::ITEMS[359];
    Item::RECORD_THIRTEEN = Item::ITEMS[2256];
    Item::RECORD_CAT = Item::ITEMS[2257];
}

} // namespace

void initializeItems()
{
    std::call_once(g_itemsInit, registerVanillaItems);
}

} // namespace net::minecraft
