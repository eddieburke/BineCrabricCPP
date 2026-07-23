/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class PumpkinBlock
extends Block {
    private boolean lit;

    protected PumpkinBlock(int id, int textureId, boolean lit) {
        super(id, Material.PUMPKIN);
        this.textureId = textureId;
        this.setTickRandomly(true);
        this.lit = lit;
    }

    public int getTexture(int side, int meta) {
        if (side == 1) {
            return this.textureId;
        }
        if (side == 0) {
            return this.textureId;
        }
        int n = this.textureId + 1 + 16;
        if (this.lit) {
            ++n;
        }
        if (meta == 2 && side == 2) {
            return n;
        }
        if (meta == 3 && side == 5) {
            return n;
        }
        if (meta == 0 && side == 3) {
            return n;
        }
        if (meta == 1 && side == 4) {
            return n;
        }
        return this.textureId + 16;
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId;
        }
        if (side == 0) {
            return this.textureId;
        }
        if (side == 3) {
            return this.textureId + 1 + 16;
        }
        return this.textureId + 16;
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y, z);
        return (n == 0 || Block.BLOCKS[n].material.isReplaceable()) && world.shouldSuffocate(x, y - 1, z);
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
        int n = MathHelper.floor((double)(placer.yaw * 4.0f / 360.0f) + 2.5) & 3;
        world.setBlockMeta(x, y, z, n);
    }
}

