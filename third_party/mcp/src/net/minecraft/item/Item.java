/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.AxeItem;
import net.minecraft.item.BedItem;
import net.minecraft.item.BoatItem;
import net.minecraft.item.BowItem;
import net.minecraft.item.BucketItem;
import net.minecraft.item.CoalItem;
import net.minecraft.item.DoorItem;
import net.minecraft.item.DyeItem;
import net.minecraft.item.EggItem;
import net.minecraft.item.FishingRodItem;
import net.minecraft.item.FlintAndSteel;
import net.minecraft.item.FoodItem;
import net.minecraft.item.HoeItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.MapItem;
import net.minecraft.item.MinecartItem;
import net.minecraft.item.MushroomStewItem;
import net.minecraft.item.MusicDiscItem;
import net.minecraft.item.PaintingItem;
import net.minecraft.item.PickaxeItem;
import net.minecraft.item.RedstoneItem;
import net.minecraft.item.SaddleItem;
import net.minecraft.item.SecondaryBlockItem;
import net.minecraft.item.SeedsItem;
import net.minecraft.item.ShearsItem;
import net.minecraft.item.ShovelItem;
import net.minecraft.item.SignItem;
import net.minecraft.item.SnowballItem;
import net.minecraft.item.StackableFoodItem;
import net.minecraft.item.SwordItem;
import net.minecraft.item.ToolMaterial;
import net.minecraft.stat.Stats;
import net.minecraft.world.World;

public class Item {
    protected static Random random = new Random();
    public static Item[] ITEMS = new Item[32000];
    public static Item IRON_SHOVEL = new ShovelItem(0, ToolMaterial.IRON).setTexturePosition(2, 5).setTranslationKey("shovelIron");
    public static Item IRON_PICKAXE = new PickaxeItem(1, ToolMaterial.IRON).setTexturePosition(2, 6).setTranslationKey("pickaxeIron");
    public static Item IRON_AXE = new AxeItem(2, ToolMaterial.IRON).setTexturePosition(2, 7).setTranslationKey("hatchetIron");
    public static Item FLINT_AND_STEEL = new FlintAndSteel(3).setTexturePosition(5, 0).setTranslationKey("flintAndSteel");
    public static Item APPLE = new FoodItem(4, 4, false).setTexturePosition(10, 0).setTranslationKey("apple");
    public static Item BOW = new BowItem(5).setTexturePosition(5, 1).setTranslationKey("bow");
    public static Item ARROW = new Item(6).setTexturePosition(5, 2).setTranslationKey("arrow");
    public static Item COAL = new CoalItem(7).setTexturePosition(7, 0).setTranslationKey("coal");
    public static Item DIAMOND = new Item(8).setTexturePosition(7, 3).setTranslationKey("emerald");
    public static Item IRON_INGOT = new Item(9).setTexturePosition(7, 1).setTranslationKey("ingotIron");
    public static Item GOLD_INGOT = new Item(10).setTexturePosition(7, 2).setTranslationKey("ingotGold");
    public static Item IRON_SWORD = new SwordItem(11, ToolMaterial.IRON).setTexturePosition(2, 4).setTranslationKey("swordIron");
    public static Item WOODEN_SWORD = new SwordItem(12, ToolMaterial.WOOD).setTexturePosition(0, 4).setTranslationKey("swordWood");
    public static Item WOODEN_SHOVEL = new ShovelItem(13, ToolMaterial.WOOD).setTexturePosition(0, 5).setTranslationKey("shovelWood");
    public static Item WOODEN_PICKAXE = new PickaxeItem(14, ToolMaterial.WOOD).setTexturePosition(0, 6).setTranslationKey("pickaxeWood");
    public static Item WOODEN_AXE = new AxeItem(15, ToolMaterial.WOOD).setTexturePosition(0, 7).setTranslationKey("hatchetWood");
    public static Item STONE_SWORD = new SwordItem(16, ToolMaterial.STONE).setTexturePosition(1, 4).setTranslationKey("swordStone");
    public static Item STONE_SHOVEL = new ShovelItem(17, ToolMaterial.STONE).setTexturePosition(1, 5).setTranslationKey("shovelStone");
    public static Item STONE_PICKAXE = new PickaxeItem(18, ToolMaterial.STONE).setTexturePosition(1, 6).setTranslationKey("pickaxeStone");
    public static Item STONE_AXE = new AxeItem(19, ToolMaterial.STONE).setTexturePosition(1, 7).setTranslationKey("hatchetStone");
    public static Item DIAMOND_SWORD = new SwordItem(20, ToolMaterial.DIAMOND).setTexturePosition(3, 4).setTranslationKey("swordDiamond");
    public static Item DIAMOND_SHOVEL = new ShovelItem(21, ToolMaterial.DIAMOND).setTexturePosition(3, 5).setTranslationKey("shovelDiamond");
    public static Item DIAMOND_PICKAXE = new PickaxeItem(22, ToolMaterial.DIAMOND).setTexturePosition(3, 6).setTranslationKey("pickaxeDiamond");
    public static Item DIAMOND_AXE = new AxeItem(23, ToolMaterial.DIAMOND).setTexturePosition(3, 7).setTranslationKey("hatchetDiamond");
    public static Item STICK = new Item(24).setTexturePosition(5, 3).setHandheld().setTranslationKey("stick");
    public static Item BOWL = new Item(25).setTexturePosition(7, 4).setTranslationKey("bowl");
    public static Item MUSHROOM_STEW = new MushroomStewItem(26, 10).setTexturePosition(8, 4).setTranslationKey("mushroomStew");
    public static Item GOLDEN_SWORD = new SwordItem(27, ToolMaterial.GOLD).setTexturePosition(4, 4).setTranslationKey("swordGold");
    public static Item GOLDEN_SHOVEL = new ShovelItem(28, ToolMaterial.GOLD).setTexturePosition(4, 5).setTranslationKey("shovelGold");
    public static Item GOLDEN_PICKAXE = new PickaxeItem(29, ToolMaterial.GOLD).setTexturePosition(4, 6).setTranslationKey("pickaxeGold");
    public static Item GOLDEN_AXE = new AxeItem(30, ToolMaterial.GOLD).setTexturePosition(4, 7).setTranslationKey("hatchetGold");
    public static Item STRING = new Item(31).setTexturePosition(8, 0).setTranslationKey("string");
    public static Item FEATHER = new Item(32).setTexturePosition(8, 1).setTranslationKey("feather");
    public static Item GUNPOWDER = new Item(33).setTexturePosition(8, 2).setTranslationKey("sulphur");
    public static Item WOODEN_HOE = new HoeItem(34, ToolMaterial.WOOD).setTexturePosition(0, 8).setTranslationKey("hoeWood");
    public static Item STONE_HOE = new HoeItem(35, ToolMaterial.STONE).setTexturePosition(1, 8).setTranslationKey("hoeStone");
    public static Item IRON_HOE = new HoeItem(36, ToolMaterial.IRON).setTexturePosition(2, 8).setTranslationKey("hoeIron");
    public static Item DIAMOND_HOE = new HoeItem(37, ToolMaterial.DIAMOND).setTexturePosition(3, 8).setTranslationKey("hoeDiamond");
    public static Item GOLDEN_HOE = new HoeItem(38, ToolMaterial.GOLD).setTexturePosition(4, 8).setTranslationKey("hoeGold");
    public static Item SEEDS = new SeedsItem(39, Block.WHEAT.id).setTexturePosition(9, 0).setTranslationKey("seeds");
    public static Item WHEAT = new Item(40).setTexturePosition(9, 1).setTranslationKey("wheat");
    public static Item BREAD = new FoodItem(41, 5, false).setTexturePosition(9, 2).setTranslationKey("bread");
    public static Item LEATHER_HELMET = new ArmorItem(42, 0, 0, 0).setTexturePosition(0, 0).setTranslationKey("helmetCloth");
    public static Item LEATHER_CHESTPLATE = new ArmorItem(43, 0, 0, 1).setTexturePosition(0, 1).setTranslationKey("chestplateCloth");
    public static Item LEATHER_LEGGINGS = new ArmorItem(44, 0, 0, 2).setTexturePosition(0, 2).setTranslationKey("leggingsCloth");
    public static Item LEATHER_BOOTS = new ArmorItem(45, 0, 0, 3).setTexturePosition(0, 3).setTranslationKey("bootsCloth");
    public static Item CHAIN_HELMET = new ArmorItem(46, 1, 1, 0).setTexturePosition(1, 0).setTranslationKey("helmetChain");
    public static Item CHAIN_CHESTPLATE = new ArmorItem(47, 1, 1, 1).setTexturePosition(1, 1).setTranslationKey("chestplateChain");
    public static Item CHAIN_LEGGINGS = new ArmorItem(48, 1, 1, 2).setTexturePosition(1, 2).setTranslationKey("leggingsChain");
    public static Item CHAIN_BOOTS = new ArmorItem(49, 1, 1, 3).setTexturePosition(1, 3).setTranslationKey("bootsChain");
    public static Item IRON_HELMET = new ArmorItem(50, 2, 2, 0).setTexturePosition(2, 0).setTranslationKey("helmetIron");
    public static Item IRON_CHESTPLATE = new ArmorItem(51, 2, 2, 1).setTexturePosition(2, 1).setTranslationKey("chestplateIron");
    public static Item IRON_LEGGINGS = new ArmorItem(52, 2, 2, 2).setTexturePosition(2, 2).setTranslationKey("leggingsIron");
    public static Item IRON_BOOTS = new ArmorItem(53, 2, 2, 3).setTexturePosition(2, 3).setTranslationKey("bootsIron");
    public static Item DIAMOND_HELMET = new ArmorItem(54, 3, 3, 0).setTexturePosition(3, 0).setTranslationKey("helmetDiamond");
    public static Item DIAMOND_CHESTPLATE = new ArmorItem(55, 3, 3, 1).setTexturePosition(3, 1).setTranslationKey("chestplateDiamond");
    public static Item DIAMOND_LEGGINGS = new ArmorItem(56, 3, 3, 2).setTexturePosition(3, 2).setTranslationKey("leggingsDiamond");
    public static Item DIAMOND_BOOTS = new ArmorItem(57, 3, 3, 3).setTexturePosition(3, 3).setTranslationKey("bootsDiamond");
    public static Item GOLDEN_HELMET = new ArmorItem(58, 1, 4, 0).setTexturePosition(4, 0).setTranslationKey("helmetGold");
    public static Item GOLDEN_CHESTPLATE = new ArmorItem(59, 1, 4, 1).setTexturePosition(4, 1).setTranslationKey("chestplateGold");
    public static Item GOLDEN_LEGGINGS = new ArmorItem(60, 1, 4, 2).setTexturePosition(4, 2).setTranslationKey("leggingsGold");
    public static Item GOLDEN_BOOTS = new ArmorItem(61, 1, 4, 3).setTexturePosition(4, 3).setTranslationKey("bootsGold");
    public static Item FLINT = new Item(62).setTexturePosition(6, 0).setTranslationKey("flint");
    public static Item RAW_PORKCHOP = new FoodItem(63, 3, true).setTexturePosition(7, 5).setTranslationKey("porkchopRaw");
    public static Item COOKED_PORKCHOP = new FoodItem(64, 8, true).setTexturePosition(8, 5).setTranslationKey("porkchopCooked");
    public static Item PAINTING = new PaintingItem(65).setTexturePosition(10, 1).setTranslationKey("painting");
    public static Item GOLDEN_APPLE = new FoodItem(66, 42, false).setTexturePosition(11, 0).setTranslationKey("appleGold");
    public static Item SIGN = new SignItem(67).setTexturePosition(10, 2).setTranslationKey("sign");
    public static Item WOODEN_DOOR = new DoorItem(68, Material.WOOD).setTexturePosition(11, 2).setTranslationKey("doorWood");
    public static Item BUCKET = new BucketItem(69, 0).setTexturePosition(10, 4).setTranslationKey("bucket");
    public static Item WATER_BUCKET = new BucketItem(70, Block.FLOWING_WATER.id).setTexturePosition(11, 4).setTranslationKey("bucketWater").setCraftingReturnItem(BUCKET);
    public static Item LAVA_BUCKET = new BucketItem(71, Block.FLOWING_LAVA.id).setTexturePosition(12, 4).setTranslationKey("bucketLava").setCraftingReturnItem(BUCKET);
    public static Item MINECART = new MinecartItem(72, 0).setTexturePosition(7, 8).setTranslationKey("minecart");
    public static Item SADDLE = new SaddleItem(73).setTexturePosition(8, 6).setTranslationKey("saddle");
    public static Item IRON_DOOR = new DoorItem(74, Material.METAL).setTexturePosition(12, 2).setTranslationKey("doorIron");
    public static Item REDSTONE = new RedstoneItem(75).setTexturePosition(8, 3).setTranslationKey("redstone");
    public static Item SNOWBALL = new SnowballItem(76).setTexturePosition(14, 0).setTranslationKey("snowball");
    public static Item BOAT = new BoatItem(77).setTexturePosition(8, 8).setTranslationKey("boat");
    public static Item LEATHER = new Item(78).setTexturePosition(7, 6).setTranslationKey("leather");
    public static Item MILK_BUCKET = new BucketItem(79, -1).setTexturePosition(13, 4).setTranslationKey("milk").setCraftingReturnItem(BUCKET);
    public static Item BRICK = new Item(80).setTexturePosition(6, 1).setTranslationKey("brick");
    public static Item CLAY = new Item(81).setTexturePosition(9, 3).setTranslationKey("clay");
    public static Item SUGAR_CANE = new SecondaryBlockItem(82, Block.SUGAR_CANE).setTexturePosition(11, 1).setTranslationKey("reeds");
    public static Item PAPER = new Item(83).setTexturePosition(10, 3).setTranslationKey("paper");
    public static Item BOOK = new Item(84).setTexturePosition(11, 3).setTranslationKey("book");
    public static Item SLIMEBALL = new Item(85).setTexturePosition(14, 1).setTranslationKey("slimeball");
    public static Item CHEST_MINECART = new MinecartItem(86, 1).setTexturePosition(7, 9).setTranslationKey("minecartChest");
    public static Item FURNACE_MINECART = new MinecartItem(87, 2).setTexturePosition(7, 10).setTranslationKey("minecartFurnace");
    public static Item EGG = new EggItem(88).setTexturePosition(12, 0).setTranslationKey("egg");
    public static Item COMPASS = new Item(89).setTexturePosition(6, 3).setTranslationKey("compass");
    public static Item FISHING_ROD = new FishingRodItem(90).setTexturePosition(5, 4).setTranslationKey("fishingRod");
    public static Item CLOCK = new Item(91).setTexturePosition(6, 4).setTranslationKey("clock");
    public static Item GLOWSTONE_DUST = new Item(92).setTexturePosition(9, 4).setTranslationKey("yellowDust");
    public static Item RAW_FISH = new FoodItem(93, 2, false).setTexturePosition(9, 5).setTranslationKey("fishRaw");
    public static Item COOKED_FISH = new FoodItem(94, 5, false).setTexturePosition(10, 5).setTranslationKey("fishCooked");
    public static Item DYE = new DyeItem(95).setTexturePosition(14, 4).setTranslationKey("dyePowder");
    public static Item BONE = new Item(96).setTexturePosition(12, 1).setTranslationKey("bone").setHandheld();
    public static Item SUGAR = new Item(97).setTexturePosition(13, 0).setTranslationKey("sugar").setHandheld();
    public static Item CAKE = new SecondaryBlockItem(98, Block.CAKE).setMaxCount(1).setTexturePosition(13, 1).setTranslationKey("cake");
    public static Item BED = new BedItem(99).setMaxCount(1).setTexturePosition(13, 2).setTranslationKey("bed");
    public static Item REPEATER = new SecondaryBlockItem(100, Block.REPEATER).setTexturePosition(6, 5).setTranslationKey("diode");
    public static Item COOKIE = new StackableFoodItem(101, 1, false, 8).setTexturePosition(12, 5).setTranslationKey("cookie");
    public static MapItem MAP = (MapItem)new MapItem(102).setTexturePosition(12, 3).setTranslationKey("map");
    public static ShearsItem SHEARS = (ShearsItem)new ShearsItem(103).setTexturePosition(13, 5).setTranslationKey("shears");
    public static Item RECORD_THIRTEEN = new MusicDiscItem(2000, "13").setTexturePosition(0, 15).setTranslationKey("record");
    public static Item RECORD_CAT = new MusicDiscItem(2001, "cat").setTexturePosition(1, 15).setTranslationKey("record");
    public final int id;
    protected int maxCount = 64;
    private int maxDamage = 0;
    protected int textureId;
    protected boolean handheld = false;
    protected boolean hasSubtypes = false;
    private Item craftingReturnItem = null;
    private String translationKey;

    protected Item(int id) {
        this.id = 256 + id;
        if (ITEMS[256 + id] != null) {
            System.out.println("CONFLICT @ " + id);
        }
        Item.ITEMS[256 + id] = this;
    }

    public Item setTextureId(int textureId) {
        this.textureId = textureId;
        return this;
    }

    public Item setMaxCount(int maxCount) {
        this.maxCount = maxCount;
        return this;
    }

    public Item setTexturePosition(int x, int y) {
        this.textureId = x + y * 16;
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(int damage) {
        return this.textureId;
    }

    @Environment(value=EnvType.CLIENT)
    public final int getTextureId(ItemStack itemStack) {
        return this.getTextureId(itemStack.getDamage());
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        return false;
    }

    public float getMiningSpeedMultiplier(ItemStack stack, Block block) {
        return 1.0f;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        return stack;
    }

    public int getMaxCount() {
        return this.maxCount;
    }

    public int getPlacementMetadata(int meta) {
        return 0;
    }

    public boolean hasSubtypes() {
        return this.hasSubtypes;
    }

    protected Item setHasSubtypes(boolean hasSubtypes) {
        this.hasSubtypes = hasSubtypes;
        return this;
    }

    public int getMaxDamage() {
        return this.maxDamage;
    }

    protected Item setMaxDamage(int maxDamage) {
        this.maxDamage = maxDamage;
        return this;
    }

    public boolean isDamageable() {
        return this.maxDamage > 0 && !this.hasSubtypes;
    }

    public boolean postHit(ItemStack stack, LivingEntity target, LivingEntity attacker) {
        return false;
    }

    public boolean postMine(ItemStack stack, int blockId, int x, int y, int z, LivingEntity miner) {
        return false;
    }

    public int getAttackDamage(Entity attackedEntity) {
        return 1;
    }

    public boolean isSuitableFor(Block block) {
        return false;
    }

    public void useOnEntity(ItemStack stack, LivingEntity entity) {
    }

    public Item setHandheld() {
        this.handheld = true;
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheld() {
        return this.handheld;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheldRod() {
        return false;
    }

    public Item setTranslationKey(String key) {
        this.translationKey = "item." + key;
        return this;
    }

    public String getTranslationKey() {
        return this.translationKey;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey(ItemStack stack) {
        return this.translationKey;
    }

    public Item setCraftingReturnItem(Item item) {
        if (this.maxCount > 1) {
            throw new IllegalArgumentException("Max stack size must be 1 for items with crafting results");
        }
        this.craftingReturnItem = item;
        return this;
    }

    public Item getCraftingReturnItem() {
        return this.craftingReturnItem;
    }

    public boolean hasCraftingReturnItem() {
        return this.craftingReturnItem != null;
    }

    public String getTranslatedName() {
        return I18n.getTranslation(this.getTranslationKey() + ".name");
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(int color) {
        return 0xFFFFFF;
    }

    public void inventoryTick(ItemStack stack, World world, Entity entity, int slot, boolean selected) {
    }

    public void onCraft(ItemStack stack, World world, PlayerEntity player) {
    }

    @Environment(value=EnvType.SERVER)
    public boolean isNetworkSynced() {
        return false;
    }

    static {
        Stats.initializeExtendedItemStats();
    }
}

