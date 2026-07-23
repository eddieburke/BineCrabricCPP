/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.block.WoolBlock;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class DyeRecipes {
    public void add(CraftingRecipeManager recipeManager) {
        for (int i = 0; i < 16; ++i) {
            recipeManager.addShapelessRecipe(new ItemStack(Block.WOOL, 1, WoolBlock.getItemMeta(i)), new ItemStack(Item.DYE, 1, i), new ItemStack(Item.ITEMS[Block.WOOL.id], 1, 0));
        }
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 11), Block.DANDELION);
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 1), Block.ROSE);
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 3, 15), Item.BONE);
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 9), new ItemStack(Item.DYE, 1, 1), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 14), new ItemStack(Item.DYE, 1, 1), new ItemStack(Item.DYE, 1, 11));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 10), new ItemStack(Item.DYE, 1, 2), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 8), new ItemStack(Item.DYE, 1, 0), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 7), new ItemStack(Item.DYE, 1, 8), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 3, 7), new ItemStack(Item.DYE, 1, 0), new ItemStack(Item.DYE, 1, 15), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 12), new ItemStack(Item.DYE, 1, 4), new ItemStack(Item.DYE, 1, 15));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 6), new ItemStack(Item.DYE, 1, 4), new ItemStack(Item.DYE, 1, 2));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 5), new ItemStack(Item.DYE, 1, 4), new ItemStack(Item.DYE, 1, 1));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 2, 13), new ItemStack(Item.DYE, 1, 5), new ItemStack(Item.DYE, 1, 9));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 3, 13), new ItemStack(Item.DYE, 1, 4), new ItemStack(Item.DYE, 1, 1), new ItemStack(Item.DYE, 1, 9));
        recipeManager.addShapelessRecipe(new ItemStack(Item.DYE, 4, 13), new ItemStack(Item.DYE, 1, 4), new ItemStack(Item.DYE, 1, 1), new ItemStack(Item.DYE, 1, 1), new ItemStack(Item.DYE, 1, 15));
    }
}

