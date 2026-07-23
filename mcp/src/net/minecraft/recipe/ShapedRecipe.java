/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.inventory.CraftingInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipe;

public class ShapedRecipe
implements CraftingRecipe {
    private int width;
    private int height;
    private ItemStack[] input;
    private ItemStack output;
    public final int outputId;

    public ShapedRecipe(int width, int height, ItemStack[] input, ItemStack output) {
        this.outputId = output.itemId;
        this.width = width;
        this.height = height;
        this.input = input;
        this.output = output;
    }

    public ItemStack getOutput() {
        return this.output;
    }

    public boolean matches(CraftingInventory craftingInventory) {
        for (int i = 0; i <= 3 - this.width; ++i) {
            for (int j = 0; j <= 3 - this.height; ++j) {
                if (this.matchesPattern(craftingInventory, i, j, true)) {
                    return true;
                }
                if (!this.matchesPattern(craftingInventory, i, j, false)) continue;
                return true;
            }
        }
        return false;
    }

    private boolean matchesPattern(CraftingInventory inv, int offsetX, int offsetY, boolean flipped) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ItemStack itemStack;
                int n = i - offsetX;
                int n2 = j - offsetY;
                ItemStack itemStack2 = null;
                if (n >= 0 && n2 >= 0 && n < this.width && n2 < this.height) {
                    itemStack2 = flipped ? this.input[this.width - n - 1 + n2 * this.width] : this.input[n + n2 * this.width];
                }
                if ((itemStack = inv.getStack(i, j)) == null && itemStack2 == null) continue;
                if (itemStack == null && itemStack2 != null || itemStack != null && itemStack2 == null) {
                    return false;
                }
                if (itemStack2.itemId != itemStack.itemId) {
                    return false;
                }
                if (itemStack2.getDamage() == -1 || itemStack2.getDamage() == itemStack.getDamage()) continue;
                return false;
            }
        }
        return true;
    }

    public ItemStack craft(CraftingInventory craftingInventory) {
        return new ItemStack(this.output.itemId, this.output.count, this.output.getDamage());
    }

    public int getSize() {
        return this.width * this.height;
    }
}

