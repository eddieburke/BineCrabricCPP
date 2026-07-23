/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.inventory;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;

public class CraftingResultInventory
implements Inventory {
    private ItemStack[] stacks = new ItemStack[1];

    public int size() {
        return 1;
    }

    public ItemStack getStack(int slot) {
        return this.stacks[slot];
    }

    public String getName() {
        return "Result";
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.stacks[slot] != null) {
            ItemStack itemStack = this.stacks[slot];
            this.stacks[slot] = null;
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.stacks[slot] = stack;
    }

    public int getMaxCountPerStack() {
        return 64;
    }

    public void markDirty() {
    }

    public boolean canPlayerUse(PlayerEntity player) {
        return true;
    }
}

