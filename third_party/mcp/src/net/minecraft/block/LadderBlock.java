/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class LadderBlock
extends Block {
    protected LadderBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON_BREAKABLE);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        float f = 0.125f;
        if (n == 2) {
            this.setBoundingBox(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
        }
        if (n == 3) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
        }
        if (n == 4) {
            this.setBoundingBox(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
        if (n == 5) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
        }
        return super.getCollisionShape(world, x, y, z);
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        float f = 0.125f;
        if (n == 2) {
            this.setBoundingBox(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
        }
        if (n == 3) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
        }
        if (n == 4) {
            this.setBoundingBox(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
        if (n == 5) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
        }
        return super.getBoundingBox(world, x, y, z);
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 8;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (world.shouldSuffocate(x - 1, y, z)) {
            return true;
        }
        if (world.shouldSuffocate(x + 1, y, z)) {
            return true;
        }
        if (world.shouldSuffocate(x, y, z - 1)) {
            return true;
        }
        return world.shouldSuffocate(x, y, z + 1);
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
        int n = world.getBlockMeta(x, y, z);
        if ((n == 0 || direction == 2) && world.shouldSuffocate(x, y, z + 1)) {
            n = 2;
        }
        if ((n == 0 || direction == 3) && world.shouldSuffocate(x, y, z - 1)) {
            n = 3;
        }
        if ((n == 0 || direction == 4) && world.shouldSuffocate(x + 1, y, z)) {
            n = 4;
        }
        if ((n == 0 || direction == 5) && world.shouldSuffocate(x - 1, y, z)) {
            n = 5;
        }
        world.setBlockMeta(x, y, z, n);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        int n = world.getBlockMeta(x, y, z);
        boolean bl = false;
        if (n == 2 && world.shouldSuffocate(x, y, z + 1)) {
            bl = true;
        }
        if (n == 3 && world.shouldSuffocate(x, y, z - 1)) {
            bl = true;
        }
        if (n == 4 && world.shouldSuffocate(x + 1, y, z)) {
            bl = true;
        }
        if (n == 5 && world.shouldSuffocate(x - 1, y, z)) {
            bl = true;
        }
        if (!bl) {
            this.dropStacks(world, x, y, z, n);
            world.setBlock(x, y, z, 0);
        }
        super.neighborUpdate(world, x, y, z, id);
    }

    public int getDroppedItemCount(Random random) {
        return 1;
    }
}

