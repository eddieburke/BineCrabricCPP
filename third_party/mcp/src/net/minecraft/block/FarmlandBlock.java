/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class FarmlandBlock
extends Block {
    protected FarmlandBlock(int id) {
        super(id, Material.SOIL);
        this.textureId = 87;
        this.setTickRandomly(true);
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
        this.setOpacity(255);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return Box.createCached(x + 0, y + 0, z + 0, x + 1, y + 1, z + 1);
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public int getTexture(int side, int meta) {
        if (side == 1 && meta > 0) {
            return this.textureId - 1;
        }
        if (side == 1) {
            return this.textureId;
        }
        return 2;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (random.nextInt(5) == 0) {
            if (this.isWaterNearby(world, x, y, z) || world.isRaining(x, y + 1, z)) {
                world.setBlockMeta(x, y, z, 7);
            } else {
                int n = world.getBlockMeta(x, y, z);
                if (n > 0) {
                    world.setBlockMeta(x, y, z, n - 1);
                } else if (!this.hasCrop(world, x, y, z)) {
                    world.setBlock(x, y, z, Block.DIRT.id);
                }
            }
        }
    }

    public void onSteppedOn(World world, int x, int y, int z, Entity entity) {
        if (world.random.nextInt(4) == 0) {
            world.setBlock(x, y, z, Block.DIRT.id);
        }
    }

    private boolean hasCrop(World world, int x, int y, int z) {
        int n = 0;
        for (int i = x - n; i <= x + n; ++i) {
            for (int j = z - n; j <= z + n; ++j) {
                if (world.getBlockId(i, y + 1, j) != Block.WHEAT.id) continue;
                return true;
            }
        }
        return false;
    }

    private boolean isWaterNearby(World world, int x, int y, int z) {
        for (int i = x - 4; i <= x + 4; ++i) {
            for (int j = y; j <= y + 1; ++j) {
                for (int k = z - 4; k <= z + 4; ++k) {
                    if (world.getMaterial(i, j, k) != Material.WATER) continue;
                    return true;
                }
            }
        }
        return false;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        super.neighborUpdate(world, x, y, z, id);
        Material material = world.getMaterial(x, y + 1, z);
        if (material.isSolid()) {
            world.setBlock(x, y, z, Block.DIRT.id);
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.DIRT.getDroppedItemId(0, random);
    }
}

