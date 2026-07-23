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
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class TorchBlock
extends Block {
    protected TorchBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON_BREAKABLE);
        this.setTickRandomly(true);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 2;
    }

    private boolean canPlaceOn(World world, int x, int y, int z) {
        return world.shouldSuffocate(x, y, z) || world.getBlockId(x, y, z) == Block.FENCE.id;
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
        if (world.shouldSuffocate(x, y, z + 1)) {
            return true;
        }
        return this.canPlaceOn(world, x, y - 1, z);
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
        int n = world.getBlockMeta(x, y, z);
        if (direction == 1 && this.canPlaceOn(world, x, y - 1, z)) {
            n = 5;
        }
        if (direction == 2 && world.shouldSuffocate(x, y, z + 1)) {
            n = 4;
        }
        if (direction == 3 && world.shouldSuffocate(x, y, z - 1)) {
            n = 3;
        }
        if (direction == 4 && world.shouldSuffocate(x + 1, y, z)) {
            n = 2;
        }
        if (direction == 5 && world.shouldSuffocate(x - 1, y, z)) {
            n = 1;
        }
        world.setBlockMeta(x, y, z, n);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        super.onTick(world, x, y, z, random);
        if (world.getBlockMeta(x, y, z) == 0) {
            this.onPlaced(world, x, y, z);
        }
    }

    public void onPlaced(World world, int x, int y, int z) {
        if (world.shouldSuffocate(x - 1, y, z)) {
            world.setBlockMeta(x, y, z, 1);
        } else if (world.shouldSuffocate(x + 1, y, z)) {
            world.setBlockMeta(x, y, z, 2);
        } else if (world.shouldSuffocate(x, y, z - 1)) {
            world.setBlockMeta(x, y, z, 3);
        } else if (world.shouldSuffocate(x, y, z + 1)) {
            world.setBlockMeta(x, y, z, 4);
        } else if (this.canPlaceOn(world, x, y - 1, z)) {
            world.setBlockMeta(x, y, z, 5);
        }
        this.breakIfCannotPlaceAt(world, x, y, z);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (this.breakIfCannotPlaceAt(world, x, y, z)) {
            int n = world.getBlockMeta(x, y, z);
            boolean bl = false;
            if (!world.shouldSuffocate(x - 1, y, z) && n == 1) {
                bl = true;
            }
            if (!world.shouldSuffocate(x + 1, y, z) && n == 2) {
                bl = true;
            }
            if (!world.shouldSuffocate(x, y, z - 1) && n == 3) {
                bl = true;
            }
            if (!world.shouldSuffocate(x, y, z + 1) && n == 4) {
                bl = true;
            }
            if (!this.canPlaceOn(world, x, y - 1, z) && n == 5) {
                bl = true;
            }
            if (bl) {
                this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
                world.setBlock(x, y, z, 0);
            }
        }
    }

    private boolean breakIfCannotPlaceAt(World world, int x, int y, int z) {
        if (!this.canPlaceAt(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
            return false;
        }
        return true;
    }

    public HitResult raycast(World world, int x, int y, int z, Vec3d startPos, Vec3d endPos) {
        int n = world.getBlockMeta(x, y, z) & 7;
        float f = 0.15f;
        if (n == 1) {
            this.setBoundingBox(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f);
        } else if (n == 2) {
            this.setBoundingBox(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f);
        } else if (n == 3) {
            this.setBoundingBox(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f);
        } else if (n == 4) {
            this.setBoundingBox(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f);
        } else {
            f = 0.1f;
            this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.6f, 0.5f + f);
        }
        return super.raycast(world, x, y, z, startPos, endPos);
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        int n = world.getBlockMeta(x, y, z);
        double d = (float)x + 0.5f;
        double d2 = (float)y + 0.7f;
        double d3 = (float)z + 0.5f;
        double d4 = 0.22f;
        double d5 = 0.27f;
        if (n == 1) {
            world.addParticle("smoke", d - d5, d2 + d4, d3, 0.0, 0.0, 0.0);
            world.addParticle("flame", d - d5, d2 + d4, d3, 0.0, 0.0, 0.0);
        } else if (n == 2) {
            world.addParticle("smoke", d + d5, d2 + d4, d3, 0.0, 0.0, 0.0);
            world.addParticle("flame", d + d5, d2 + d4, d3, 0.0, 0.0, 0.0);
        } else if (n == 3) {
            world.addParticle("smoke", d, d2 + d4, d3 - d5, 0.0, 0.0, 0.0);
            world.addParticle("flame", d, d2 + d4, d3 - d5, 0.0, 0.0, 0.0);
        } else if (n == 4) {
            world.addParticle("smoke", d, d2 + d4, d3 + d5, 0.0, 0.0, 0.0);
            world.addParticle("flame", d, d2 + d4, d3 + d5, 0.0, 0.0, 0.0);
        } else {
            world.addParticle("smoke", d, d2, d3, 0.0, 0.0, 0.0);
            world.addParticle("flame", d, d2, d3, 0.0, 0.0, 0.0);
        }
    }
}

