/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.screen;

import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;

public class DispenserScreenHandler
extends ScreenHandler {
    private DispenserBlockEntity dispenserBlockEntity;

    public DispenserScreenHandler(Inventory playerInventory, DispenserBlockEntity dispenserBlockEntity) {
        int n;
        int n2;
        this.dispenserBlockEntity = dispenserBlockEntity;
        for (n2 = 0; n2 < 3; ++n2) {
            for (n = 0; n < 3; ++n) {
                this.addSlot(new Slot(dispenserBlockEntity, n + n2 * 3, 62 + n * 18, 17 + n2 * 18));
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
    }

    public boolean canUse(PlayerEntity player) {
        return this.dispenserBlockEntity.canPlayerUse(player);
    }
}

