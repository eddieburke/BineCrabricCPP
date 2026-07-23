/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class RedstoneItem
extends Item {
    public RedstoneItem(int i) {
        super(i);
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (world.getBlockId(x, y, z) != Block.SNOW.id) {
            if (side == 0) {
                --y;
            }
            if (side == 1) {
                ++y;
            }
            if (side == 2) {
                --z;
            }
            if (side == 3) {
                ++z;
            }
            if (side == 4) {
                --x;
            }
            if (side == 5) {
                ++x;
            }
            if (!world.isAir(x, y, z)) {
                return false;
            }
        }
        if (Block.REDSTONE_WIRE.canPlaceAt(world, x, y, z)) {
            --stack.count;
            world.setBlock(x, y, z, Block.REDSTONE_WIRE.id);
        }
        return true;
    }
}

