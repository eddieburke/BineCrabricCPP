/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.screen.slot;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;

public class Slot {
    private final int index;
    private final Inventory inventory;
    public int id;
    public int x;
    public int y;

    public Slot(Inventory inventory, int index, int x, int y) {
        this.inventory = inventory;
        this.index = index;
        this.x = x;
        this.y = y;
    }

    public void onTakeItem(ItemStack stack) {
        this.markDirty();
    }

    public boolean canInsert(ItemStack stack) {
        return true;
    }

    public ItemStack getStack() {
        return this.inventory.getStack(this.index);
    }

    public boolean hasStack() {
        return this.getStack() != null;
    }

    public void setStack(ItemStack stack) {
        this.inventory.setStack(this.index, stack);
        this.markDirty();
    }

    public void markDirty() {
        this.inventory.markDirty();
    }

    public int getMaxItemCount() {
        return this.inventory.getMaxCountPerStack();
    }

    @Environment(value=EnvType.CLIENT)
    public int getBackgroundTextureId() {
        return -1;
    }

    public ItemStack takeStack(int amount) {
        return this.inventory.removeStack(this.index, amount);
    }

    @Environment(value=EnvType.SERVER)
    public boolean equals(Inventory inventory, int index) {
        return inventory == this.inventory && index == this.index;
    }
}

