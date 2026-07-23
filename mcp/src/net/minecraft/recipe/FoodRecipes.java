/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class FoodRecipes {
    public void add(CraftingRecipeManager recipeManager) {
        recipeManager.addShapedRecipe(new ItemStack(Item.MUSHROOM_STEW), "Y", "X", "#", Character.valueOf('X'), Block.BROWN_MUSHROOM, Character.valueOf('Y'), Block.RED_MUSHROOM, Character.valueOf('#'), Item.BOWL);
        recipeManager.addShapedRecipe(new ItemStack(Item.MUSHROOM_STEW), "Y", "X", "#", Character.valueOf('X'), Block.RED_MUSHROOM, Character.valueOf('Y'), Block.BROWN_MUSHROOM, Character.valueOf('#'), Item.BOWL);
        recipeManager.addShapedRecipe(new ItemStack(Item.COOKIE, 8), "#X#", Character.valueOf('X'), new ItemStack(Item.DYE, 1, 3), Character.valueOf('#'), Item.WHEAT);
    }
}

