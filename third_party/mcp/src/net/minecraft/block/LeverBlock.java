/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class LeverBlock
extends Block {
    protected LeverBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON_BREAKABLE);
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
        return 12;
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
        if (side == 1 && world.shouldSuffocate(x, y - 1, z)) {
            return true;
        }
        if (side == 2 && world.shouldSuffocate(x, y, z + 1)) {
            return true;
        }
        if (side == 3 && world.shouldSuffocate(x, y, z - 1)) {
            return true;
        }
        if (side == 4 && world.shouldSuffocate(x + 1, y, z)) {
            return true;
        }
        return side == 5 && world.shouldSuffocate(x - 1, y, z);
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
        return world.shouldSuffocate(x, y - 1, z);
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = n & 8;
        n &= 7;
        n = -1;
        if (direction == 1 && world.shouldSuffocate(x, y - 1, z)) {
            n = 5 + world.random.nextInt(2);
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
        if (n == -1) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
            return;
        }
        world.setBlockMeta(x, y, z, n + n2);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (this.breakIfCannotPlaceAt(world, x, y, z)) {
            int n = world.getBlockMeta(x, y, z) & 7;
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
            if (!world.shouldSuffocate(x, y - 1, z) && n == 5) {
                bl = true;
            }
            if (!world.shouldSuffocate(x, y - 1, z) && n == 6) {
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

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z) & 7;
        float f = 0.1875f;
        if (n == 1) {
            this.setBoundingBox(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f);
        } else if (n == 2) {
            this.setBoundingBox(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f);
        } else if (n == 3) {
            this.setBoundingBox(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f);
        } else if (n == 4) {
            this.setBoundingBox(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f);
        } else {
            f = 0.25f;
            this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.6f, 0.5f + f);
        }
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.onUse(world, x, y, z, player);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return true;
        }
        int n = world.getBlockMeta(x, y, z);
        int n2 = n & 7;
        int n3 = 8 - (n & 8);
        world.setBlockMeta(x, y, z, n2 + n3);
        world.setBlocksDirty(x, y, z, x, y, z);
        world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "random.click", 0.3f, n3 > 0 ? 0.6f : 0.5f);
        world.notifyNeighbors(x, y, z, this.id);
        if (n2 == 1) {
            world.notifyNeighbors(x - 1, y, z, this.id);
        } else if (n2 == 2) {
            world.notifyNeighbors(x + 1, y, z, this.id);
        } else if (n2 == 3) {
            world.notifyNeighbors(x, y, z - 1, this.id);
        } else if (n2 == 4) {
            world.notifyNeighbors(x, y, z + 1, this.id);
        } else {
            world.notifyNeighbors(x, y - 1, z, this.id);
        }
        return true;
    }

    public void onBreak(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) > 0) {
            world.notifyNeighbors(x, y, z, this.id);
            int n2 = n & 7;
            if (n2 == 1) {
                world.notifyNeighbors(x - 1, y, z, this.id);
            } else if (n2 == 2) {
                world.notifyNeighbors(x + 1, y, z, this.id);
            } else if (n2 == 3) {
                world.notifyNeighbors(x, y, z - 1, this.id);
            } else if (n2 == 4) {
                world.notifyNeighbors(x, y, z + 1, this.id);
            } else {
                world.notifyNeighbors(x, y - 1, z, this.id);
            }
        }
        super.onBreak(world, x, y, z);
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        return (blockView.getBlockMeta(x, y, z) & 8) > 0;
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) == 0) {
            return false;
        }
        int n2 = n & 7;
        if (n2 == 6 && direction == 1) {
            return true;
        }
        if (n2 == 5 && direction == 1) {
            return true;
        }
        if (n2 == 4 && direction == 2) {
            return true;
        }
        if (n2 == 3 && direction == 3) {
            return true;
        }
        if (n2 == 2 && direction == 4) {
            return true;
        }
        return n2 == 1 && direction == 5;
    }

    public boolean canEmitRedstonePower() {
        return true;
    }
}

