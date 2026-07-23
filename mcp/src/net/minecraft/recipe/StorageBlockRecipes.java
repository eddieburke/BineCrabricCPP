/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class StorageBlockRecipes {
    private Object[][] items = new Object[][]{{Block.GOLD_BLOCK, new ItemStack(Item.GOLD_INGOT, 9)}, {Block.IRON_BLOCK, new ItemStack(Item.IRON_INGOT, 9)}, {Block.DIAMOND_BLOCK, new ItemStack(Item.DIAMOND, 9)}, {Block.LAPIS_BLOCK, new ItemStack(Item.DYE, 9, 4)}};

    public void add(CraftingRecipeManager recipeManager) {
        for (int i = 0; i < this.items.length; ++i) {
            Block block = (Block)this.items[i][0];
            ItemStack itemStack = (ItemStack)this.items[i][1];
            recipeManager.addShapedRecipe(new ItemStack(block), "###", "###", "###", Character.valueOf('#'), itemStack);
            recipeManager.addShapedRecipe(itemStack, "#", Character.valueOf('#'), block);
        }
    }
}

