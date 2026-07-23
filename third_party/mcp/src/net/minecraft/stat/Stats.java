/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.stat;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipe;
import net.minecraft.recipe.CraftingRecipeManager;
import net.minecraft.recipe.SmeltingRecipeManager;
import net.minecraft.stat.ItemOrBlockStat;
import net.minecraft.stat.SimpleStat;
import net.minecraft.stat.Stat;

public class Stats {
    protected static Map ID_TO_STAT = new HashMap();
    public static List ALL_STATS = new ArrayList();
    public static List GENERAL_STATS = new ArrayList();
    public static List ITEM_STATS = new ArrayList();
    public static List BLOCK_MINED_STATS = new ArrayList();
    public static Stat START_GAME = new SimpleStat(1000, I18n.getTranslation("stat.startGame")).localOnly().addStat();
    public static Stat CREATE_WORLD = new SimpleStat(1001, I18n.getTranslation("stat.createWorld")).localOnly().addStat();
    public static Stat LOAD_WORLD = new SimpleStat(1002, I18n.getTranslation("stat.loadWorld")).localOnly().addStat();
    public static Stat JOIN_MULTIPLAYER = new SimpleStat(1003, I18n.getTranslation("stat.joinMultiplayer")).localOnly().addStat();
    public static Stat LEAVE_GAME = new SimpleStat(1004, I18n.getTranslation("stat.leaveGame")).localOnly().addStat();
    public static Stat PLAY_ONE_MINUTE = new SimpleStat(1100, I18n.getTranslation("stat.playOneMinute"), Stat.TIME_PROVIDER).localOnly().addStat();
    public static Stat WALK_ONE_CM = new SimpleStat(2000, I18n.getTranslation("stat.walkOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat SWIM_ONE_CM = new SimpleStat(2001, I18n.getTranslation("stat.swimOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat FALL_ONE_CM = new SimpleStat(2002, I18n.getTranslation("stat.fallOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat CLIMB_ONE_CM = new SimpleStat(2003, I18n.getTranslation("stat.climbOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat FLY_ONE_CM = new SimpleStat(2004, I18n.getTranslation("stat.flyOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat DIVE_ONE_CM = new SimpleStat(2005, I18n.getTranslation("stat.diveOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat MINECART_ONE_CM = new SimpleStat(2006, I18n.getTranslation("stat.minecartOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat BOAT_ONE_CM = new SimpleStat(2007, I18n.getTranslation("stat.boatOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat PIG_ONE_CM = new SimpleStat(2008, I18n.getTranslation("stat.pigOneCm"), Stat.DISTANCE_PROVIDER).localOnly().addStat();
    public static Stat JUMP = new SimpleStat(2010, I18n.getTranslation("stat.jump")).localOnly().addStat();
    public static Stat DROP = new SimpleStat(2011, I18n.getTranslation("stat.drop")).localOnly().addStat();
    public static Stat DAMAGE_DEALT = new SimpleStat(2020, I18n.getTranslation("stat.damageDealt")).addStat();
    public static Stat DAMAGE_TAKEN = new SimpleStat(2021, I18n.getTranslation("stat.damageTaken")).addStat();
    public static Stat DEATHS = new SimpleStat(2022, I18n.getTranslation("stat.deaths")).addStat();
    public static Stat MOB_KILLS = new SimpleStat(2023, I18n.getTranslation("stat.mobKills")).addStat();
    public static Stat PLAYER_KILLS = new SimpleStat(2024, I18n.getTranslation("stat.playerKills")).addStat();
    public static Stat FISH_CAUGHT = new SimpleStat(2025, I18n.getTranslation("stat.fishCaught")).addStat();
    public static Stat[] MINE_BLOCK = Stats.initBlocksMined("stat.mineBlock", 0x1000000);
    public static Stat[] CRAFTED;
    public static Stat[] USED;
    public static Stat[] BROKEN;
    private static boolean hasBasicItemStatsInitialized;
    private static boolean hasExtendedItemStatsInitialized;

    public static void initialize() {
    }

    public static void initializeItemStats() {
        USED = Stats.initItemsUsedStats(USED, "stat.useItem", 0x1020000, 0, Block.BLOCKS.length);
        BROKEN = Stats.initializeBrokenItemStats(BROKEN, "stat.breakItem", 0x1030000, 0, Block.BLOCKS.length);
        hasBasicItemStatsInitialized = true;
        Stats.initializeCraftedItemStats();
    }

    public static void initializeExtendedItemStats() {
        USED = Stats.initItemsUsedStats(USED, "stat.useItem", 0x1020000, Block.BLOCKS.length, 32000);
        BROKEN = Stats.initializeBrokenItemStats(BROKEN, "stat.breakItem", 0x1030000, Block.BLOCKS.length, 32000);
        hasExtendedItemStatsInitialized = true;
        Stats.initializeCraftedItemStats();
    }

    public static void initializeCraftedItemStats() {
        if (!hasBasicItemStatsInitialized || !hasExtendedItemStatsInitialized) {
            return;
        }
        HashSet<Integer> hashSet = new HashSet<Integer>();
        for (CraftingRecipe object : CraftingRecipeManager.getInstance().getRecipes()) {
            hashSet.add(object.getOutput().itemId);
        }
        for (ItemStack itemStack : SmeltingRecipeManager.getInstance().getRecipes().values()) {
            hashSet.add(itemStack.itemId);
        }
        CRAFTED = new Stat[32000];
        for (Integer n : hashSet) {
            if (Item.ITEMS[n] == null) continue;
            String string = I18n.getTranslation("stat.craftItem", Item.ITEMS[n].getTranslatedName());
            Stats.CRAFTED[n.intValue()] = new ItemOrBlockStat(0x1010000 + n, string, n).addStat();
        }
        Stats.linkAlternateBlockStats(CRAFTED);
    }

    private static Stat[] initBlocksMined(String name, int id) {
        Stat[] statArray = new Stat[256];
        for (int i = 0; i < 256; ++i) {
            if (Block.BLOCKS[i] == null || !Block.BLOCKS[i].isTrackingStatistics()) continue;
            String string = I18n.getTranslation(name, Block.BLOCKS[i].getTranslatedName());
            statArray[i] = new ItemOrBlockStat(id + i, string, i).addStat();
            BLOCK_MINED_STATS.add((ItemOrBlockStat)statArray[i]);
        }
        Stats.linkAlternateBlockStats(statArray);
        return statArray;
    }

    private static Stat[] initItemsUsedStats(Stat[] stats, String name, int id, int minId, int maxId) {
        if (stats == null) {
            stats = new Stat[32000];
        }
        for (int i = minId; i < maxId; ++i) {
            if (Item.ITEMS[i] == null) continue;
            String string = I18n.getTranslation(name, Item.ITEMS[i].getTranslatedName());
            stats[i] = new ItemOrBlockStat(id + i, string, i).addStat();
            if (i < Block.BLOCKS.length) continue;
            ITEM_STATS.add((ItemOrBlockStat)stats[i]);
        }
        Stats.linkAlternateBlockStats(stats);
        return stats;
    }

    private static Stat[] initializeBrokenItemStats(Stat[] statsArray, String translationKey, int offset, int start, int end) {
        if (statsArray == null) {
            statsArray = new Stat[32000];
        }
        for (int i = start; i < end; ++i) {
            if (Item.ITEMS[i] == null || !Item.ITEMS[i].isDamageable()) continue;
            String string = I18n.getTranslation(translationKey, Item.ITEMS[i].getTranslatedName());
            statsArray[i] = new ItemOrBlockStat(offset + i, string, i).addStat();
        }
        Stats.linkAlternateBlockStats(statsArray);
        return statsArray;
    }

    private static void linkAlternateBlockStats(Stat[] statsArray) {
        Stats.linkBlocks(statsArray, Block.WATER.id, Block.FLOWING_WATER.id);
        Stats.linkBlocks(statsArray, Block.LAVA.id, Block.LAVA.id);
        Stats.linkBlocks(statsArray, Block.JACK_O_LANTERN.id, Block.PUMPKIN.id);
        Stats.linkBlocks(statsArray, Block.LIT_FURNACE.id, Block.FURNACE.id);
        Stats.linkBlocks(statsArray, Block.LIT_REDSTONE_ORE.id, Block.REDSTONE_ORE.id);
        Stats.linkBlocks(statsArray, Block.POWERED_REPEATER.id, Block.REPEATER.id);
        Stats.linkBlocks(statsArray, Block.LIT_REDSTONE_TORCH.id, Block.REDSTONE_TORCH.id);
        Stats.linkBlocks(statsArray, Block.RED_MUSHROOM.id, Block.BROWN_MUSHROOM.id);
        Stats.linkBlocks(statsArray, Block.DOUBLE_SLAB.id, Block.SLAB.id);
        Stats.linkBlocks(statsArray, Block.GRASS_BLOCK.id, Block.DIRT.id);
        Stats.linkBlocks(statsArray, Block.FARMLAND.id, Block.DIRT.id);
    }

    private static void linkBlocks(Stat[] statsArray, int sourceId, int targetId) {
        if (statsArray[sourceId] != null && statsArray[targetId] == null) {
            statsArray[targetId] = statsArray[sourceId];
            return;
        }
        ALL_STATS.remove(statsArray[sourceId]);
        BLOCK_MINED_STATS.remove(statsArray[sourceId]);
        GENERAL_STATS.remove(statsArray[sourceId]);
        statsArray[sourceId] = statsArray[targetId];
    }

    @Environment(value=EnvType.CLIENT)
    public static Stat getStatById(int id) {
        return (Stat)ID_TO_STAT.get(id);
    }

    static {
        Achievements.initialize();
        hasBasicItemStatsInitialized = false;
        hasExtendedItemStatsInitialized = false;
    }
}

