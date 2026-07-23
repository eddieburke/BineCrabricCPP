/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.screen;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;

public class GenericContainerScreenHandler
extends ScreenHandler {
    private Inventory inventory;
    private int rows;

    public GenericContainerScreenHandler(Inventory playerInventory, Inventory inventory) {
        int n;
        int n2;
        this.inventory = inventory;
        this.rows = inventory.size() / 9;
        int n3 = (this.rows - 4) * 18;
        for (n2 = 0; n2 < this.rows; ++n2) {
            for (n = 0; n < 9; ++n) {
                this.addSlot(new Slot(inventory, n + n2 * 9, 8 + n * 18, 18 + n2 * 18));
            }
        }
        for (n2 = 0; n2 < 3; ++n2) {
            for (n = 0; n < 9; ++n) {
                this.addSlot(new Slot(playerInventory, n + n2 * 9 + 9, 8 + n * 18, 103 + n2 * 18 + n3));
            }
        }
        for (n2 = 0; n2 < 9; ++n2) {
            this.addSlot(new Slot(playerInventory, n2, 8 + n2 * 18, 161 + n3));
        }
    }

    public boolean canUse(PlayerEntity player) {
        return this.inventory.canPlayerUse(player);
    }

    public ItemStack quickMove(int slot) {
        ItemStack itemStack = null;
        Slot slot2 = (Slot)this.slots.get(slot);
        if (slot2 != null && slot2.hasStack()) {
            ItemStack itemStack2 = slot2.getStack();
            itemStack = itemStack2.copy();
            if (slot < this.rows * 9) {
                this.insertItem(itemStack2, this.rows * 9, this.slots.size(), true);
            } else {
                this.insertItem(itemStack2, 0, this.rows * 9, false);
            }
            if (itemStack2.count == 0) {
                slot2.setStack(null);
            } else {
                slot2.markDirty();
            }
        }
        return itemStack;
    }
}

