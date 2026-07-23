/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.achievement;

import java.util.ArrayList;
import java.util.List;
import net.minecraft.achievement.Achievement;
import net.minecraft.block.Block;
import net.minecraft.item.Item;

public class Achievements {
    public static int minColumn;
    public static int minRow;
    public static int maxColumn;
    public static int maxRow;
    public static List ACHIEVEMENTS;
    public static Achievement OPEN_INVENTORY;
    public static Achievement MINE_WOOD;
    public static Achievement CRAFT_WORKBENCH;
    public static Achievement CRAFT_PICKAXE;
    public static Achievement CRAFT_FURNACE;
    public static Achievement ACQUIRE_IRON;
    public static Achievement CRAFT_HOE;
    public static Achievement CRAFT_BREAD;
    public static Achievement CRAFT_CAKE;
    public static Achievement CRAFT_STONE_PICKAXE;
    public static Achievement COOK_FISH;
    public static Achievement CRAFT_RAIL;
    public static Achievement CRAFT_SWORD;
    public static Achievement KILL_ENEMY;
    public static Achievement KILL_COW;
    public static Achievement FLY_PIG;

    public static void initialize() {
    }

    static {
        ACHIEVEMENTS = new ArrayList();
        OPEN_INVENTORY = new Achievement(0, "openInventory", 0, 0, Item.BOOK, null).localOnly().addStat();
        MINE_WOOD = new Achievement(1, "mineWood", 2, 1, Block.LOG, OPEN_INVENTORY).addStat();
        CRAFT_WORKBENCH = new Achievement(2, "buildWorkBench", 4, -1, Block.CRAFTING_TABLE, MINE_WOOD).addStat();
        CRAFT_PICKAXE = new Achievement(3, "buildPickaxe", 4, 2, Item.WOODEN_PICKAXE, CRAFT_WORKBENCH).addStat();
        CRAFT_FURNACE = new Achievement(4, "buildFurnace", 3, 4, Block.LIT_FURNACE, CRAFT_PICKAXE).addStat();
        ACQUIRE_IRON = new Achievement(5, "acquireIron", 1, 4, Item.IRON_INGOT, CRAFT_FURNACE).addStat();
        CRAFT_HOE = new Achievement(6, "buildHoe", 2, -3, Item.WOODEN_HOE, CRAFT_WORKBENCH).addStat();
        CRAFT_BREAD = new Achievement(7, "makeBread", -1, -3, Item.BREAD, CRAFT_HOE).addStat();
        CRAFT_CAKE = new Achievement(8, "bakeCake", 0, -5, Item.CAKE, CRAFT_HOE).addStat();
        CRAFT_STONE_PICKAXE = new Achievement(9, "buildBetterPickaxe", 6, 2, Item.STONE_PICKAXE, CRAFT_PICKAXE).addStat();
        COOK_FISH = new Achievement(10, "cookFish", 2, 6, Item.COOKED_FISH, CRAFT_FURNACE).addStat();
        CRAFT_RAIL = new Achievement(11, "onARail", 2, 3, Block.RAIL, ACQUIRE_IRON).challenge().addStat();
        CRAFT_SWORD = new Achievement(12, "buildSword", 6, -1, Item.WOODEN_SWORD, CRAFT_WORKBENCH).addStat();
        KILL_ENEMY = new Achievement(13, "killEnemy", 8, -1, Item.BONE, CRAFT_SWORD).addStat();
        KILL_COW = new Achievement(14, "killCow", 7, -3, Item.LEATHER, CRAFT_SWORD).addStat();
        FLY_PIG = new Achievement(15, "flyPig", 8, -4, Item.SADDLE, KILL_COW).challenge().addStat();
        System.out.println(ACHIEVEMENTS.size() + " achievements");
    }
}

