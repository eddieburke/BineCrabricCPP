/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.screen;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.inventory.CraftingInventory;
import net.minecraft.inventory.CraftingResultInventory;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.CraftingResultSlot;
import net.minecraft.screen.slot.Slot;
import net.minecraft.world.World;

public class CraftingScreenHandler
extends ScreenHandler {
    public CraftingInventory input = new CraftingInventory(this, 3, 3);
    public Inventory result = new CraftingResultInventory();
    private World world;
    private int x;
    private int y;
    private int z;

    public CraftingScreenHandler(PlayerInventory playerInventory, World world, int x, int y, int z) {
        int n;
        int n2;
        this.world = world;
        this.x = x;
        this.y = y;
        this.z = z;
        this.addSlot(new CraftingResultSlot(playerInventory.player, this.input, this.result, 0, 124, 35));
        for (n2 = 0; n2 < 3; ++n2) {
            for (n = 0; n < 3; ++n) {
                this.addSlot(new Slot(this.input, n + n2 * 3, 30 + n * 18, 17 + n2 * 18));
            }
        }
        for (n2 = 0; n2 < 3; ++n2) {
            for (n = 0; n < 9; ++n) {
                this.addSlot(new Slot(playerInventory, n + n2 * 9 + 9, 8 + n * 18, 84 + n2 * 18));
            }
        }
        for (n2 = 0; n2 < 9; ++n2) {
            this.addSlot(new Slot(playerInventory, n2, 8 + n2 * 18, 142));
        }
        this.onSlotUpdate(this.input);
    }

    public void onSlotUpdate(Inventory inventory) {
        this.result.setStack(0, CraftingRecipeManager.getInstance().craft(this.input));
    }

    public void onClosed(PlayerEntity player) {
        super.onClosed(player);
        if (this.world.isRemote) {
            return;
        }
        for (int i = 0; i < 9; ++i) {
            ItemStack itemStack = this.input.getStack(i);
            if (itemStack == null) continue;
            player.dropItem(itemStack);
        }
    }

    public boolean canUse(PlayerEntity player) {
        if (this.world.getBlockId(this.x, this.y, this.z) != Block.CRAFTING_TABLE.id) {
            return false;
        }
        return !(player.getSquaredDistance((double)this.x + 0.5, (double)this.y + 0.5, (double)this.z + 0.5) > 64.0);
    }

    public ItemStack quickMove(int slot) {
        ItemStack itemStack = null;
        Slot slot2 = (Slot)this.slots.get(slot);
        if (slot2 != null && slot2.hasStack()) {
            ItemStack itemStack2 = slot2.getStack();
            itemStack = itemStack2.copy();
            if (slot == 0) {
                this.insertItem(itemStack2, 10, 46, true);
            } else if (slot >= 10 && slot < 37) {
                this.insertItem(itemStack2, 37, 46, false);
            } else if (slot >= 37 && slot < 46) {
                this.insertItem(itemStack2, 10, 37, false);
            } else {
                this.insertItem(itemStack2, 10, 46, false);
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

