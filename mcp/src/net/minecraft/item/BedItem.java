/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.BedBlock;
import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class BedItem
extends Item {
    public BedItem(int i) {
        super(i);
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (side != 1) {
            return false;
        }
        ++y;
        BedBlock bedBlock = (BedBlock)Block.BED;
        int n = MathHelper.floor((double)(user.yaw * 4.0f / 360.0f) + 0.5) & 3;
        int n2 = 0;
        int n3 = 0;
        if (n == 0) {
            n3 = 1;
        }
        if (n == 1) {
            n2 = -1;
        }
        if (n == 2) {
            n3 = -1;
        }
        if (n == 3) {
            n2 = 1;
        }
        if (world.isAir(x, y, z) && world.isAir(x + n2, y, z + n3) && world.shouldSuffocate(x, y - 1, z) && world.shouldSuffocate(x + n2, y - 1, z + n3)) {
            world.setBlock(x, y, z, bedBlock.id, n);
            world.setBlock(x + n2, y, z + n3, bedBlock.id, n + 8);
            --stack.count;
            return true;
        }
        return false;
    }
}

