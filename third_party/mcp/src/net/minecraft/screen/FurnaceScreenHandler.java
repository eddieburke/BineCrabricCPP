/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.screen;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.ScreenHandlerListener;
import net.minecraft.screen.slot.FurnaceOutputSlot;
import net.minecraft.screen.slot.Slot;

public class FurnaceScreenHandler
extends ScreenHandler {
    private FurnaceBlockEntity furnaceBlockEntity;
    private int cookTime = 0;
    private int burnTime = 0;
    private int fuelTime = 0;

    public FurnaceScreenHandler(PlayerInventory playerInventory, FurnaceBlockEntity furnaceBlockEntity) {
        int n;
        this.furnaceBlockEntity = furnaceBlockEntity;
        this.addSlot(new Slot(furnaceBlockEntity, 0, 56, 17));
        this.addSlot(new Slot(furnaceBlockEntity, 1, 56, 53));
        this.addSlot(new FurnaceOutputSlot(playerInventory.player, furnaceBlockEntity, 2, 116, 35));
        for (n = 0; n < 3; ++n) {
            for (int i = 0; i < 9; ++i) {
                this.addSlot(new Slot(playerInventory, i + n * 9 + 9, 8 + i * 18, 84 + n * 18));
            }
        }
        for (n = 0; n < 9; ++n) {
            this.addSlot(new Slot(playerInventory, n, 8 + n * 18, 142));
        }
    }

    @Environment(value=EnvType.SERVER)
    public void addListener(ScreenHandlerListener listener) {
        super.addListener(listener);
        listener.onPropertyUpdate(this, 0, this.furnaceBlockEntity.cookTime);
        listener.onPropertyUpdate(this, 1, this.furnaceBlockEntity.burnTime);
        listener.onPropertyUpdate(this, 2, this.furnaceBlockEntity.fuelTime);
    }

    public void sendContentUpdates() {
        super.sendContentUpdates();
        for (int i = 0; i < this.listeners.size(); ++i) {
            ScreenHandlerListener screenHandlerListener = (ScreenHandlerListener)this.listeners.get(i);
            if (this.cookTime != this.furnaceBlockEntity.cookTime) {
                screenHandlerListener.onPropertyUpdate(this, 0, this.furnaceBlockEntity.cookTime);
            }
            if (this.burnTime != this.furnaceBlockEntity.burnTime) {
                screenHandlerListener.onPropertyUpdate(this, 1, this.furnaceBlockEntity.burnTime);
            }
            if (this.fuelTime == this.furnaceBlockEntity.fuelTime) continue;
            screenHandlerListener.onPropertyUpdate(this, 2, this.furnaceBlockEntity.fuelTime);
        }
        this.cookTime = this.furnaceBlockEntity.cookTime;
        this.burnTime = this.furnaceBlockEntity.burnTime;
        this.fuelTime = this.furnaceBlockEntity.fuelTime;
    }

    @Environment(value=EnvType.CLIENT)
    public void setProperty(int id, int value) {
        if (id == 0) {
            this.furnaceBlockEntity.cookTime = value;
        }
        if (id == 1) {
            this.furnaceBlockEntity.burnTime = value;
        }
        if (id == 2) {
            this.furnaceBlockEntity.fuelTime = value;
        }
    }

    public boolean canUse(PlayerEntity player) {
        return this.furnaceBlockEntity.canPlayerUse(player);
    }

    public ItemStack quickMove(int slot) {
        ItemStack itemStack = null;
        Slot slot2 = (Slot)this.slots.get(slot);
        if (slot2 != null && slot2.hasStack()) {
            ItemStack itemStack2 = slot2.getStack();
            itemStack = itemStack2.copy();
            if (slot == 2) {
                this.insertItem(itemStack2, 3, 39, true);
            } else if (slot >= 3 && slot < 30) {
                this.insertItem(itemStack2, 30, 39, false);
            } else if (slot >= 30 && slot < 39) {
                this.insertItem(itemStack2, 3, 30, false);
            } else {
                this.insertItem(itemStack2, 3, 39, false);
            }
            if (itemStack2.count == 0) {
                slot2.setStack(null);
            } else {
                slot2.markDirty();
            }
            if (itemStack2.count != itemStack.count) {
                slot2.onTakeItem(itemStack2);
            } else {
                return null;
            }
        }
        return itemStack;
    }
}

