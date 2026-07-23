/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.inventory;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;

public class DoubleInventory
implements Inventory {
    private String name;
    private Inventory first;
    private Inventory second;

    public DoubleInventory(String name, Inventory first, Inventory second) {
        this.name = name;
        this.first = first;
        this.second = second;
    }

    public int size() {
        return this.first.size() + this.second.size();
    }

    public String getName() {
        return this.name;
    }

    public ItemStack getStack(int slot) {
        if (slot >= this.first.size()) {
            return this.second.getStack(slot - this.first.size());
        }
        return this.first.getStack(slot);
    }

    public ItemStack removeStack(int slot, int amount) {
        if (slot >= this.first.size()) {
            return this.second.removeStack(slot - this.first.size(), amount);
        }
        return this.first.removeStack(slot, amount);
    }

    public void setStack(int slot, ItemStack stack) {
        if (slot >= this.first.size()) {
            this.second.setStack(slot - this.first.size(), stack);
        } else {
            this.first.setStack(slot, stack);
        }
    }

    public int getMaxCountPerStack() {
        return this.first.getMaxCountPerStack();
    }

    public void markDirty() {
        this.first.markDirty();
        this.second.markDirty();
    }

    public boolean canPlayerUse(PlayerEntity player) {
        return this.first.canPlayerUse(player) && this.second.canPlayerUse(player);
    }
}

