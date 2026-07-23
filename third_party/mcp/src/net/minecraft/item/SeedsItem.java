/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class SeedsItem
extends Item {
    private int cropBlockId;

    public SeedsItem(int id, int cropBlockId) {
        super(id);
        this.cropBlockId = cropBlockId;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (side != 1) {
            return false;
        }
        int n = world.getBlockId(x, y, z);
        if (n == Block.FARMLAND.id && world.isAir(x, y + 1, z)) {
            world.setBlock(x, y + 1, z, this.cropBlockId);
            --stack.count;
            return true;
        }
        return false;
    }
}

