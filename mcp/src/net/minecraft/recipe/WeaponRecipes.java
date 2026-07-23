/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class WeaponRecipes {
    private String[][] patterns = new String[][]{{"X", "X", "#"}};
    private Object[][] items = new Object[][]{{Block.PLANKS, Block.COBBLESTONE, Item.IRON_INGOT, Item.DIAMOND, Item.GOLD_INGOT}, {Item.WOODEN_SWORD, Item.STONE_SWORD, Item.IRON_SWORD, Item.DIAMOND_SWORD, Item.GOLDEN_SWORD}};

    public void add(CraftingRecipeManager recipeManager) {
        for (int i = 0; i < this.items[0].length; ++i) {
            Object object = this.items[0][i];
            for (int j = 0; j < this.items.length - 1; ++j) {
                Item item = (Item)this.items[j + 1][i];
                recipeManager.addShapedRecipe(new ItemStack(item), this.patterns[j], Character.valueOf('#'), Item.STICK, Character.valueOf('X'), object);
            }
        }
        recipeManager.addShapedRecipe(new ItemStack(Item.BOW, 1), " #X", "# X", " #X", Character.valueOf('X'), Item.STRING, Character.valueOf('#'), Item.STICK);
        recipeManager.addShapedRecipe(new ItemStack(Item.ARROW, 4), "X", "#", "Y", Character.valueOf('Y'), Item.FEATHER, Character.valueOf('X'), Item.FLINT, Character.valueOf('#'), Item.STICK);
    }
}

