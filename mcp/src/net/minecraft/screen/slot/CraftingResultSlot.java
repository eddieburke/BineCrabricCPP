/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.screen.slot;

import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.slot.Slot;

public class CraftingResultSlot
extends Slot {
    private final Inventory input;
    private PlayerEntity player;

    public CraftingResultSlot(PlayerEntity player, Inventory input, Inventory inventory, int index, int x, int y) {
        super(inventory, index, x, y);
        this.player = player;
        this.input = input;
    }

    public boolean canInsert(ItemStack stack) {
        return false;
    }

    public void onTakeItem(ItemStack stack) {
        stack.onCraft(this.player.world, this.player);
        if (stack.itemId == Block.CRAFTING_TABLE.id) {
            this.player.increaseStat(Achievements.CRAFT_WORKBENCH, 1);
        } else if (stack.itemId == Item.WOODEN_PICKAXE.id) {
            this.player.increaseStat(Achievements.CRAFT_PICKAXE, 1);
        } else if (stack.itemId == Block.FURNACE.id) {
            this.player.increaseStat(Achievements.CRAFT_FURNACE, 1);
        } else if (stack.itemId == Item.WOODEN_HOE.id) {
            this.player.increaseStat(Achievements.CRAFT_HOE, 1);
        } else if (stack.itemId == Item.BREAD.id) {
            this.player.increaseStat(Achievements.CRAFT_BREAD, 1);
        } else if (stack.itemId == Item.CAKE.id) {
            this.player.increaseStat(Achievements.CRAFT_CAKE, 1);
        } else if (stack.itemId == Item.STONE_PICKAXE.id) {
            this.player.increaseStat(Achievements.CRAFT_STONE_PICKAXE, 1);
        } else if (stack.itemId == Item.WOODEN_SWORD.id) {
            this.player.increaseStat(Achievements.CRAFT_SWORD, 1);
        }
        for (int i = 0; i < this.input.size(); ++i) {
            ItemStack itemStack = this.input.getStack(i);
            if (itemStack == null) continue;
            this.input.removeStack(i, 1);
            if (!itemStack.getItem().hasCraftingReturnItem()) continue;
            this.input.setStack(i, new ItemStack(itemStack.getItem().getCraftingReturnItem()));
        }
    }
}

