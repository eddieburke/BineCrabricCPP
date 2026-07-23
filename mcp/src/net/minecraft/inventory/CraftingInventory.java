/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.inventory;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;

public class CraftingInventory
implements Inventory {
    private ItemStack[] stacks;
    private int width;
    private ScreenHandler handler;

    public CraftingInventory(ScreenHandler handler, int width, int height) {
        int n = width * height;
        this.stacks = new ItemStack[n];
        this.handler = handler;
        this.width = width;
    }

    public int size() {
        return this.stacks.length;
    }

    public ItemStack getStack(int slot) {
        if (slot >= this.size()) {
            return null;
        }
        return this.stacks[slot];
    }

    public ItemStack getStack(int x, int y) {
        if (x < 0 || x >= this.width) {
            return null;
        }
        int n = x + y * this.width;
        return this.getStack(n);
    }

    public String getName() {
        return "Crafting";
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.stacks[slot] != null) {
            if (this.stacks[slot].count <= amount) {
                ItemStack itemStack = this.stacks[slot];
                this.stacks[slot] = null;
                this.handler.onSlotUpdate(this);
                return itemStack;
            }
            ItemStack itemStack = this.stacks[slot].split(amount);
            if (this.stacks[slot].count == 0) {
                this.stacks[slot] = null;
            }
            this.handler.onSlotUpdate(this);
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.stacks[slot] = stack;
        this.handler.onSlotUpdate(this);
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

