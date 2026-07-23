/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class ArmorRecipes {
    private String[][] patterns = new String[][]{{"XXX", "X X"}, {"X X", "XXX", "XXX"}, {"XXX", "X X", "X X"}, {"X X", "X X"}};
    private Object[][] items = new Object[][]{{Item.LEATHER, Block.FIRE, Item.IRON_INGOT, Item.DIAMOND, Item.GOLD_INGOT}, {Item.LEATHER_HELMET, Item.CHAIN_HELMET, Item.IRON_HELMET, Item.DIAMOND_HELMET, Item.GOLDEN_HELMET}, {Item.LEATHER_CHESTPLATE, Item.CHAIN_CHESTPLATE, Item.IRON_CHESTPLATE, Item.DIAMOND_CHESTPLATE, Item.GOLDEN_CHESTPLATE}, {Item.LEATHER_LEGGINGS, Item.CHAIN_LEGGINGS, Item.IRON_LEGGINGS, Item.DIAMOND_LEGGINGS, Item.GOLDEN_LEGGINGS}, {Item.LEATHER_BOOTS, Item.CHAIN_BOOTS, Item.IRON_BOOTS, Item.DIAMOND_BOOTS, Item.GOLDEN_BOOTS}};

    public void add(CraftingRecipeManager recipeManager) {
        for (int i = 0; i < this.items[0].length; ++i) {
            Object object = this.items[0][i];
            for (int j = 0; j < this.items.length - 1; ++j) {
                Item item = (Item)this.items[j + 1][i];
                recipeManager.addShapedRecipe(new ItemStack(item), this.patterns[j], Character.valueOf('X'), object);
            }
        }
    }
}

