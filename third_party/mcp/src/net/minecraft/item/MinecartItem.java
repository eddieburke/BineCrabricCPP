/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.RailBlock;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class MinecartItem
extends Item {
    public int type;

    public MinecartItem(int id, int type) {
        super(id);
        this.maxCount = 1;
        this.type = type;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        int n = world.getBlockId(x, y, z);
        if (RailBlock.isRail(n)) {
            if (!world.isRemote) {
                world.spawnEntity(new MinecartEntity(world, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, this.type));
            }
            --stack.count;
            return true;
        }
        return false;
    }
}

