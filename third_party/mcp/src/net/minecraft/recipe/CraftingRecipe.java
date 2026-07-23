/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import net.minecraft.inventory.CraftingInventory;
import net.minecraft.item.ItemStack;

public interface CraftingRecipe {
    public boolean matches(CraftingInventory var1);

    public ItemStack craft(CraftingInventory var1);

    public int getSize();

    public ItemStack getOutput();
}

