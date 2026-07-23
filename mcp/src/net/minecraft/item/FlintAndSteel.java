/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class FlintAndSteel
extends Item {
    public FlintAndSteel(int i) {
        super(i);
        this.maxCount = 1;
        this.setMaxDamage(64);
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        int n;
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
        if ((n = world.getBlockId(x, y, z)) == 0) {
            world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "fire.ignite", 1.0f, random.nextFloat() * 0.4f + 0.8f);
            world.setBlock(x, y, z, Block.FIRE.id);
        }
        stack.damage(1, user);
        return true;
    }
}

