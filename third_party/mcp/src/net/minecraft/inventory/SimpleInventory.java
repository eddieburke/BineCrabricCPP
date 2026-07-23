/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.inventory;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.inventory.InventoryListener;
import net.minecraft.item.ItemStack;

@Environment(value=EnvType.CLIENT)
public class SimpleInventory
implements Inventory {
    private String name;
    private int size;
    private ItemStack[] stacks;
    private List listeners;

    public SimpleInventory(String name, int size) {
        this.name = name;
        this.size = size;
        this.stacks = new ItemStack[size];
    }

    public ItemStack getStack(int slot) {
        return this.stacks[slot];
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.stacks[slot] != null) {
            if (this.stacks[slot].count <= amount) {
                ItemStack itemStack = this.stacks[slot];
                this.stacks[slot] = null;
                this.markDirty();
                return itemStack;
            }
            ItemStack itemStack = this.stacks[slot].split(amount);
            if (this.stacks[slot].count == 0) {
                this.stacks[slot] = null;
            }
            this.markDirty();
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.stacks[slot] = stack;
        if (stack != null && stack.count > this.getMaxCountPerStack()) {
            stack.count = this.getMaxCountPerStack();
        }
        this.markDirty();
    }

    public int size() {
        return this.size;
    }

    public String getName() {
        return this.name;
    }

    public int getMaxCountPerStack() {
        return 64;
    }

    public void markDirty() {
        if (this.listeners != null) {
            for (int i = 0; i < this.listeners.size(); ++i) {
                ((InventoryListener)this.listeners.get(i)).onInventoryChanged(this);
            }
        }
    }

    public boolean canPlayerUse(PlayerEntity player) {
        return true;
    }
}

