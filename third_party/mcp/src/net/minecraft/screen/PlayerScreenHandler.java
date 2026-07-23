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
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipeManager;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.CraftingResultSlot;
import net.minecraft.screen.slot.Slot;

public class PlayerScreenHandler
extends ScreenHandler {
    public CraftingInventory craftingInput = new CraftingInventory(this, 2, 2);
    public Inventory craftingResult = new CraftingResultInventory();
    public boolean isLocal = false;

    public PlayerScreenHandler(PlayerInventory inventory) {
        this(inventory, true);
    }

    public PlayerScreenHandler(PlayerInventory inventory, boolean isLocal) {
        int n;
        int n2;
        this.isLocal = isLocal;
        this.addSlot(new CraftingResultSlot(inventory.player, this.craftingInput, this.craftingResult, 0, 144, 36));
        for (n2 = 0; n2 < 2; ++n2) {
            for (n = 0; n < 2; ++n) {
                this.addSlot(new Slot(this.craftingInput, n + n2 * 2, 88 + n * 18, 26 + n2 * 18));
            }
        }
        for (n2 = 0; n2 < 4; ++n2) {
            n = n2;
            this.addSlot(new Slot(this, inventory, inventory.size() - 1 - n2, 8, 8 + n2 * 18, n){
                final /* synthetic */ int armorSlotIndex;
                final /* synthetic */ PlayerScreenHandler outerHandler;
                {
                    this.outerHandler = arg;
                    this.armorSlotIndex = l;
                    super(arg2, i, j, k);
                }

                public int getMaxItemCount() {
                    return 1;
                }

                public boolean canInsert(ItemStack stack) {
                    if (stack.getItem() instanceof ArmorItem) {
                        return ((ArmorItem)stack.getItem()).equipmentSlot == this.armorSlotIndex;
                    }
                    if (stack.getItem().id == Block.PUMPKIN.id) {
                        return this.armorSlotIndex == 0;
                    }
                    return false;
                }
            });
        }
        for (n2 = 0; n2 < 3; ++n2) {
            for (n = 0; n < 9; ++n) {
                this.addSlot(new Slot(inventory, n + (n2 + 1) * 9, 8 + n * 18, 84 + n2 * 18));
            }
        }
        for (n2 = 0; n2 < 9; ++n2) {
            this.addSlot(new Slot(inventory, n2, 8 + n2 * 18, 142));
        }
        this.onSlotUpdate(this.craftingInput);
    }

    public void onSlotUpdate(Inventory inventory) {
        this.craftingResult.setStack(0, CraftingRecipeManager.getInstance().craft(this.craftingInput));
    }

    public void onClosed(PlayerEntity player) {
        super.onClosed(player);
        for (int i = 0; i < 4; ++i) {
            ItemStack itemStack = this.craftingInput.getStack(i);
            if (itemStack == null) continue;
            player.dropItem(itemStack);
            this.craftingInput.setStack(i, null);
        }
    }

    public boolean canUse(PlayerEntity player) {
        return true;
    }

    public ItemStack quickMove(int slot) {
        ItemStack itemStack = null;
        Slot slot2 = (Slot)this.slots.get(slot);
        if (slot2 != null && slot2.hasStack()) {
            ItemStack itemStack2 = slot2.getStack();
            itemStack = itemStack2.copy();
            if (slot == 0) {
                this.insertItem(itemStack2, 9, 45, true);
            } else if (slot >= 9 && slot < 36) {
                this.insertItem(itemStack2, 36, 45, false);
            } else if (slot >= 36 && slot < 45) {
                this.insertItem(itemStack2, 9, 36, false);
            } else {
                this.insertItem(itemStack2, 9, 45, false);
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

