/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.screen.slot;

import net.minecraft.achievement.Achievements;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.slot.Slot;

public class FurnaceOutputSlot
extends Slot {
    private PlayerEntity player;

    public FurnaceOutputSlot(PlayerEntity player, Inventory inventory, int index, int x, int y) {
        super(inventory, index, x, y);
        this.player = player;
    }

    public boolean canInsert(ItemStack stack) {
        return false;
    }

    public void onTakeItem(ItemStack stack) {
        stack.onCraft(this.player.world, this.player);
        if (stack.itemId == Item.IRON_INGOT.id) {
            this.player.increaseStat(Achievements.ACQUIRE_IRON, 1);
        }
        if (stack.itemId == Item.COOKED_FISH.id) {
            this.player.increaseStat(Achievements.COOK_FISH, 1);
        }
        super.onTakeItem(stack);
    }
}

