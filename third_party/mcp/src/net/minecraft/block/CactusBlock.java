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
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class CactusBlock
extends Block {
    protected CactusBlock(int id, int textureId) {
        super(id, textureId, Material.CACTUS);
        this.setTickRandomly(true);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isAir(x, y + 1, z)) {
            int n = 1;
            while (world.getBlockId(x, y - n, z) == this.id) {
                ++n;
            }
            if (n < 3) {
                int n2 = world.getBlockMeta(x, y, z);
                if (n2 == 15) {
                    world.setBlock(x, y + 1, z, this.id);
                    world.setBlockMeta(x, y, z, 0);
                } else {
                    world.setBlockMeta(x, y, z, n2 + 1);
                }
            }
        }
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        float f = 0.0625f;
        return Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, (float)(y + 1) - f, (float)(z + 1) - f);
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        float f = 0.0625f;
        return Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, y + 1, (float)(z + 1) - f);
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId - 1;
        }
        if (side == 0) {
            return this.textureId + 1;
        }
        return this.textureId;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 13;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (!super.canPlaceAt(world, x, y, z)) {
            return false;
        }
        return this.canGrow(world, x, y, z);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (!this.canGrow(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }

    public boolean canGrow(World world, int x, int y, int z) {
        if (world.getMaterial(x - 1, y, z).isSolid()) {
            return false;
        }
        if (world.getMaterial(x + 1, y, z).isSolid()) {
            return false;
        }
        if (world.getMaterial(x, y, z - 1).isSolid()) {
            return false;
        }
        if (world.getMaterial(x, y, z + 1).isSolid()) {
            return false;
        }
        int n = world.getBlockId(x, y - 1, z);
        return n == Block.CACTUS.id || n == Block.SAND.id;
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        entity.damage(null, 1);
    }
}

