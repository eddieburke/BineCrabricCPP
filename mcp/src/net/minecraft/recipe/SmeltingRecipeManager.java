/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import java.util.HashMap;
import java.util.Map;
import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

public class SmeltingRecipeManager {
    private static final SmeltingRecipeManager INSTANCE = new SmeltingRecipeManager();
    private Map recipes = new HashMap();

    public static final SmeltingRecipeManager getInstance() {
        return INSTANCE;
    }

    private SmeltingRecipeManager() {
        this.addRecipe(Block.IRON_ORE.id, new ItemStack(Item.IRON_INGOT));
        this.addRecipe(Block.GOLD_ORE.id, new ItemStack(Item.GOLD_INGOT));
        this.addRecipe(Block.DIAMOND_ORE.id, new ItemStack(Item.DIAMOND));
        this.addRecipe(Block.SAND.id, new ItemStack(Block.GLASS));
        this.addRecipe(Item.RAW_PORKCHOP.id, new ItemStack(Item.COOKED_PORKCHOP));
        this.addRecipe(Item.RAW_FISH.id, new ItemStack(Item.COOKED_FISH));
        this.addRecipe(Block.COBBLESTONE.id, new ItemStack(Block.STONE));
        this.addRecipe(Item.CLAY.id, new ItemStack(Item.BRICK));
        this.addRecipe(Block.CACTUS.id, new ItemStack(Item.DYE, 1, 2));
        this.addRecipe(Block.LOG.id, new ItemStack(Item.COAL, 1, 1));
    }

    public void addRecipe(int inputId, ItemStack output) {
        this.recipes.put(inputId, output);
    }

    public ItemStack craft(int inputId) {
        return (ItemStack)this.recipes.get(inputId);
    }

    public Map getRecipes() {
        return this.recipes;
    }
}

