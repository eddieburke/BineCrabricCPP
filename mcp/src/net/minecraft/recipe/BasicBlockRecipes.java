/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class BasicBlockRecipes {
    public void add(CraftingRecipeManager recipeManager) {
        recipeManager.addShapedRecipe(new ItemStack(Block.CHEST), "###", "# #", "###", Character.valueOf('#'), Block.PLANKS);
        recipeManager.addShapedRecipe(new ItemStack(Block.FURNACE), "###", "# #", "###", Character.valueOf('#'), Block.COBBLESTONE);
        recipeManager.addShapedRecipe(new ItemStack(Block.CRAFTING_TABLE), "##", "##", Character.valueOf('#'), Block.PLANKS);
        recipeManager.addShapedRecipe(new ItemStack(Block.SANDSTONE), "##", "##", Character.valueOf('#'), Block.SAND);
    }
}

