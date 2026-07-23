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
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class ButtonBlock
extends Block {
    protected ButtonBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON_BREAKABLE);
        this.setTickRandomly(true);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public int getTickRate() {
        return 20;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
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
        return world.shouldSuffocate(x, y, z + 1);
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = n & 8;
        n &= 7;
        n = direction == 2 && world.shouldSuffocate(x, y, z + 1) ? 4 : (direction == 3 && world.shouldSuffocate(x, y, z - 1) ? 3 : (direction == 4 && world.shouldSuffocate(x + 1, y, z) ? 2 : (direction == 5 && world.shouldSuffocate(x - 1, y, z) ? 1 : this.getPlacementSide(world, x, y, z))));
        world.setBlockMeta(x, y, z, n + n2);
    }

    private int getPlacementSide(World world, int x, int y, int z) {
        if (world.shouldSuffocate(x - 1, y, z)) {
            return 1;
        }
        if (world.shouldSuffocate(x + 1, y, z)) {
            return 2;
        }
        if (world.shouldSuffocate(x, y, z - 1)) {
            return 3;
        }
        if (world.shouldSuffocate(x, y, z + 1)) {
            return 4;
        }
        return 1;
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
        int n = blockView.getBlockMeta(x, y, z);
        int n2 = n & 7;
        boolean bl = (n & 8) > 0;
        float f = 0.375f;
        float f2 = 0.625f;
        float f3 = 0.1875f;
        float f4 = 0.125f;
        if (bl) {
            f4 = 0.0625f;
        }
        if (n2 == 1) {
            this.setBoundingBox(0.0f, f, 0.5f - f3, f4, f2, 0.5f + f3);
        } else if (n2 == 2) {
            this.setBoundingBox(1.0f - f4, f, 0.5f - f3, 1.0f, f2, 0.5f + f3);
        } else if (n2 == 3) {
            this.setBoundingBox(0.5f - f3, f, 0.0f, 0.5f + f3, f2, f4);
        } else if (n2 == 4) {
            this.setBoundingBox(0.5f - f3, f, 1.0f - f4, 0.5f + f3, f2, 1.0f);
        }
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.onUse(world, x, y, z, player);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = n & 7;
        int n3 = 8 - (n & 8);
        if (n3 == 0) {
            return true;
        }
        world.setBlockMeta(x, y, z, n2 + n3);
        world.setBlocksDirty(x, y, z, x, y, z);
        world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "random.click", 0.3f, 0.6f);
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
        world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
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

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) == 0) {
            return;
        }
        world.setBlockMeta(x, y, z, n & 7);
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
        world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "random.click", 0.3f, 0.5f);
        world.setBlocksDirty(x, y, z, x, y, z);
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
        float f = 0.1875f;
        float f2 = 0.125f;
        float f3 = 0.125f;
        this.setBoundingBox(0.5f - f, 0.5f - f2, 0.5f - f3, 0.5f + f, 0.5f + f2, 0.5f + f3);
    }
}

