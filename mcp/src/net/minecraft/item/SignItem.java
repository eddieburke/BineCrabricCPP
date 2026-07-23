/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class SignItem
extends Item {
    public SignItem(int i) {
        super(i);
        this.maxCount = 1;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (side == 0) {
            return false;
        }
        if (!world.getMaterial(x, y, z).isSolid()) {
            return false;
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
        if (!Block.SIGN.canPlaceAt(world, x, y, z)) {
            return false;
        }
        if (side == 1) {
            world.setBlock(x, y, z, Block.SIGN.id, MathHelper.floor((double)((user.yaw + 180.0f) * 16.0f / 360.0f) + 0.5) & 0xF);
        } else {
            world.setBlock(x, y, z, Block.WALL_SIGN.id, side);
        }
        --stack.count;
        SignBlockEntity signBlockEntity = (SignBlockEntity)world.getBlockEntity(x, y, z);
        if (signBlockEntity != null) {
            user.openEditSignScreen(signBlockEntity);
        }
        return true;
    }
}

