/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class PaintingItem
extends Item {
    public PaintingItem(int i) {
        super(i);
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        PaintingEntity paintingEntity;
        if (side == 0) {
            return false;
        }
        if (side == 1) {
            return false;
        }
        int n = 0;
        if (side == 4) {
            n = 1;
        }
        if (side == 3) {
            n = 2;
        }
        if (side == 5) {
            n = 3;
        }
        if ((paintingEntity = new PaintingEntity(world, x, y, z, n)).canStayAttached()) {
            if (!world.isRemote) {
                world.spawnEntity(paintingEntity);
            }
            --stack.count;
        }
        return true;
    }
}

