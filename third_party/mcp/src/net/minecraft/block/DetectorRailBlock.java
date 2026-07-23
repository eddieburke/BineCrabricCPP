/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.List;
import java.util.Random;
import net.minecraft.block.RailBlock;
import net.minecraft.entity.Entity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class DetectorRailBlock
extends RailBlock {
    public DetectorRailBlock(int id, int textureId) {
        super(id, textureId, true);
        this.setTickRandomly(true);
    }

    public int getTickRate() {
        return 20;
    }

    public boolean canEmitRedstonePower() {
        return true;
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) != 0) {
            return;
        }
        this.updatePoweredStatus(world, x, y, z, n);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) == 0) {
            return;
        }
        this.updatePoweredStatus(world, x, y, z, n);
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        return (blockView.getBlockMeta(x, y, z) & 8) != 0;
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        if ((world.getBlockMeta(x, y, z) & 8) == 0) {
            return false;
        }
        return direction == 1;
    }

    private void updatePoweredStatus(World world, int x, int y, int z, int meta) {
        boolean bl = (meta & 8) != 0;
        boolean bl2 = false;
        float f = 0.125f;
        List list = world.collectEntitiesByClass(MinecartEntity.class, Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, (double)y + 0.25, (float)(z + 1) - f));
        if (list.size() > 0) {
            bl2 = true;
        }
        if (bl2 && !bl) {
            world.setBlockMeta(x, y, z, meta | 8);
            world.notifyNeighbors(x, y, z, this.id);
            world.notifyNeighbors(x, y - 1, z, this.id);
            world.setBlocksDirty(x, y, z, x, y, z);
        }
        if (!bl2 && bl) {
            world.setBlockMeta(x, y, z, meta & 7);
            world.notifyNeighbors(x, y, z, this.id);
            world.notifyNeighbors(x, y - 1, z, this.id);
            world.setBlocksDirty(x, y, z, x, y, z);
        }
        if (bl2) {
            world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
        }
    }
}

