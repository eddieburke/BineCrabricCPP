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
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class RepeaterBlock
extends Block {
    public static final double[] RENDER_OFFSET = new double[]{-0.0625, 0.0625, 0.1875, 0.3125};
    private static final int[] DELAY = new int[]{1, 2, 3, 4};
    private final boolean lit;

    protected RepeaterBlock(int id, boolean lit) {
        super(id, 6, Material.PISTON_BREAKABLE);
        this.lit = lit;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (!world.shouldSuffocate(x, y - 1, z)) {
            return false;
        }
        return super.canPlaceAt(world, x, y, z);
    }

    public boolean canGrow(World world, int x, int y, int z) {
        if (!world.shouldSuffocate(x, y - 1, z)) {
            return false;
        }
        return super.canGrow(world, x, y, z);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        int n = world.getBlockMeta(x, y, z);
        boolean bl = this.isPowered(world, x, y, z, n);
        if (this.lit && !bl) {
            world.setBlock(x, y, z, Block.REPEATER.id, n);
        } else if (!this.lit) {
            world.setBlock(x, y, z, Block.POWERED_REPEATER.id, n);
            if (!bl) {
                int n2 = (n & 0xC) >> 2;
                world.scheduleBlockUpdate(x, y, z, Block.POWERED_REPEATER.id, DELAY[n2] * 2);
            }
        }
    }

    public int getTexture(int side, int meta) {
        if (side == 0) {
            if (this.lit) {
                return 99;
            }
            return 115;
        }
        if (side == 1) {
            if (this.lit) {
                return 147;
            }
            return 131;
        }
        return 5;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        return side != 0 && side != 1;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 15;
    }

    public int getTexture(int side) {
        return this.getTexture(side, 0);
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        return this.isEmittingRedstonePowerInDirection(world, x, y, z, direction);
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        if (!this.lit) {
            return false;
        }
        int n = blockView.getBlockMeta(x, y, z) & 3;
        if (n == 0 && direction == 3) {
            return true;
        }
        if (n == 1 && direction == 4) {
            return true;
        }
        if (n == 2 && direction == 2) {
            return true;
        }
        return n == 3 && direction == 5;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (!this.canGrow(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        boolean bl = this.isPowered(world, x, y, z, n);
        int n2 = (n & 0xC) >> 2;
        if (this.lit && !bl) {
            world.scheduleBlockUpdate(x, y, z, this.id, DELAY[n2] * 2);
        } else if (!this.lit && bl) {
            world.scheduleBlockUpdate(x, y, z, this.id, DELAY[n2] * 2);
        }
    }

    private boolean isPowered(World world, int x, int y, int z, int meta) {
        int n = meta & 3;
        switch (n) {
            case 0: {
                return world.isEmittingRedstonePowerInDirection(x, y, z + 1, 3) || world.getBlockId(x, y, z + 1) == Block.REDSTONE_WIRE.id && world.getBlockMeta(x, y, z + 1) > 0;
            }
            case 2: {
                return world.isEmittingRedstonePowerInDirection(x, y, z - 1, 2) || world.getBlockId(x, y, z - 1) == Block.REDSTONE_WIRE.id && world.getBlockMeta(x, y, z - 1) > 0;
            }
            case 3: {
                return world.isEmittingRedstonePowerInDirection(x + 1, y, z, 5) || world.getBlockId(x + 1, y, z) == Block.REDSTONE_WIRE.id && world.getBlockMeta(x + 1, y, z) > 0;
            }
            case 1: {
                return world.isEmittingRedstonePowerInDirection(x - 1, y, z, 4) || world.getBlockId(x - 1, y, z) == Block.REDSTONE_WIRE.id && world.getBlockMeta(x - 1, y, z) > 0;
            }
        }
        return false;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = (n & 0xC) >> 2;
        n2 = n2 + 1 << 2 & 0xC;
        world.setBlockMeta(x, y, z, n2 | n & 3);
        return true;
    }

    public boolean canEmitRedstonePower() {
        return false;
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
        int n = ((MathHelper.floor((double)(placer.yaw * 4.0f / 360.0f) + 0.5) & 3) + 2) % 4;
        world.setBlockMeta(x, y, z, n);
        boolean bl = this.isPowered(world, x, y, z, n);
        if (bl) {
            world.scheduleBlockUpdate(x, y, z, this.id, 1);
        }
    }

    public void onPlaced(World world, int x, int y, int z) {
        world.notifyNeighbors(x + 1, y, z, this.id);
        world.notifyNeighbors(x - 1, y, z, this.id);
        world.notifyNeighbors(x, y, z + 1, this.id);
        world.notifyNeighbors(x, y, z - 1, this.id);
        world.notifyNeighbors(x, y - 1, z, this.id);
        world.notifyNeighbors(x, y + 1, z, this.id);
    }

    public boolean isOpaque() {
        return false;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.REPEATER.id;
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        if (!this.lit) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        double d = (double)((float)x + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
        double d2 = (double)((float)y + 0.4f) + (double)(random.nextFloat() - 0.5f) * 0.2;
        double d3 = (double)((float)z + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
        double d4 = 0.0;
        double d5 = 0.0;
        if (random.nextInt(2) == 0) {
            switch (n & 3) {
                case 0: {
                    d5 = -0.3125;
                    break;
                }
                case 2: {
                    d5 = 0.3125;
                    break;
                }
                case 3: {
                    d4 = -0.3125;
                    break;
                }
                case 1: {
                    d4 = 0.3125;
                }
            }
        } else {
            int n2 = (n & 0xC) >> 2;
            switch (n & 3) {
                case 0: {
                    d5 = RENDER_OFFSET[n2];
                    break;
                }
                case 2: {
                    d5 = -RENDER_OFFSET[n2];
                    break;
                }
                case 3: {
                    d4 = RENDER_OFFSET[n2];
                    break;
                }
                case 1: {
                    d4 = -RENDER_OFFSET[n2];
                }
            }
        }
        world.addParticle("reddust", d + d4, d2, d3 + d5, 0.0, 0.0, 0.0);
    }
}

