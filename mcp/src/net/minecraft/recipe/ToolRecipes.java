/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.block.Block;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;

public class ToolRecipes {
    private String[][] patterns = new String[][]{{"XXX", " # ", " # "}, {"X", "#", "#"}, {"XX", "X#", " #"}, {"XX", " #", " #"}};
    private Object[][] items = new Object[][]{{Block.PLANKS, Block.COBBLESTONE, Item.IRON_INGOT, Item.DIAMOND, Item.GOLD_INGOT}, {Item.WOODEN_PICKAXE, Item.STONE_PICKAXE, Item.IRON_PICKAXE, Item.DIAMOND_PICKAXE, Item.GOLDEN_PICKAXE}, {Item.WOODEN_SHOVEL, Item.STONE_SHOVEL, Item.IRON_SHOVEL, Item.DIAMOND_SHOVEL, Item.GOLDEN_SHOVEL}, {Item.WOODEN_AXE, Item.STONE_AXE, Item.IRON_AXE, Item.DIAMOND_AXE, Item.GOLDEN_AXE}, {Item.WOODEN_HOE, Item.STONE_HOE, Item.IRON_HOE, Item.DIAMOND_HOE, Item.GOLDEN_HOE}};

    public void add(CraftingRecipeManager recipeManager) {
        for (int i = 0; i < this.items[0].length; ++i) {
            Object object = this.items[0][i];
            for (int j = 0; j < this.items.length - 1; ++j) {
                Item item = (Item)this.items[j + 1][i];
                recipeManager.addShapedRecipe(new ItemStack(item), this.patterns[j], Character.valueOf('#'), Item.STICK, Character.valueOf('X'), object);
            }
        }
        recipeManager.addShapedRecipe(new ItemStack(Item.SHEARS), " #", "# ", Character.valueOf('#'), Item.IRON_INGOT);
    }
}

