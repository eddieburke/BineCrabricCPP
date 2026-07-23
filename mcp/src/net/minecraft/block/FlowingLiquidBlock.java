/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.material.Material;
import net.minecraft.world.World;

public class FlowingLiquidBlock
extends LiquidBlock {
    int adjacentSources = 0;
    boolean[] spread = new boolean[4];
    int[] distanceToGap = new int[4];

    protected FlowingLiquidBlock(int i, Material arg) {
        super(i, arg);
    }

    private void convertToSource(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        world.setBlockWithoutNotifyingNeighbors(x, y, z, this.id + 1, n);
        world.setBlocksDirty(x, y, z, x, y, z);
        world.blockUpdateEvent(x, y, z);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        int n;
        int n2 = this.getLiquidState(world, x, y, z);
        int n3 = 1;
        if (this.material == Material.LAVA && !world.dimension.evaporatesWater) {
            n3 = 2;
        }
        boolean bl = true;
        if (n2 > 0) {
            int n4 = -100;
            this.adjacentSources = 0;
            n4 = this.getLowestDepth(world, x - 1, y, z, n4);
            n4 = this.getLowestDepth(world, x + 1, y, z, n4);
            n4 = this.getLowestDepth(world, x, y, z - 1, n4);
            n = (n4 = this.getLowestDepth(world, x, y, z + 1, n4)) + n3;
            if (n >= 8 || n4 < 0) {
                n = -1;
            }
            if (this.getLiquidState(world, x, y + 1, z) >= 0) {
                int n5 = this.getLiquidState(world, x, y + 1, z);
                n = n5 >= 8 ? n5 : n5 + 8;
            }
            if (this.adjacentSources >= 2 && this.material == Material.WATER) {
                if (world.getMaterial(x, y - 1, z).isSolid()) {
                    n = 0;
                } else if (world.getMaterial(x, y - 1, z) == this.material && world.getBlockMeta(x, y, z) == 0) {
                    n = 0;
                }
            }
            if (this.material == Material.LAVA && n2 < 8 && n < 8 && n > n2 && random.nextInt(4) != 0) {
                n = n2;
                bl = false;
            }
            if (n != n2) {
                n2 = n;
                if (n2 < 0) {
                    world.setBlock(x, y, z, 0);
                } else {
                    world.setBlockMeta(x, y, z, n2);
                    world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
                    world.notifyNeighbors(x, y, z, this.id);
                }
            } else if (bl) {
                this.convertToSource(world, x, y, z);
            }
        } else {
            this.convertToSource(world, x, y, z);
        }
        if (this.canSpreadTo(world, x, y - 1, z)) {
            if (n2 >= 8) {
                world.setBlock(x, y - 1, z, this.id, n2);
            } else {
                world.setBlock(x, y - 1, z, this.id, n2 + 8);
            }
        } else if (n2 >= 0 && (n2 == 0 || this.isLiquidBreaking(world, x, y - 1, z))) {
            boolean[] blArray = this.getSpread(world, x, y, z);
            n = n2 + n3;
            if (n2 >= 8) {
                n = 1;
            }
            if (n >= 8) {
                return;
            }
            if (blArray[0]) {
                this.spreadTo(world, x - 1, y, z, n);
            }
            if (blArray[1]) {
                this.spreadTo(world, x + 1, y, z, n);
            }
            if (blArray[2]) {
                this.spreadTo(world, x, y, z - 1, n);
            }
            if (blArray[3]) {
                this.spreadTo(world, x, y, z + 1, n);
            }
        }
    }

    private void spreadTo(World world, int x, int y, int z, int depth) {
        if (this.canSpreadTo(world, x, y, z)) {
            int n = world.getBlockId(x, y, z);
            if (n > 0) {
                if (this.material == Material.LAVA) {
                    this.fizz(world, x, y, z);
                } else {
                    Block.BLOCKS[n].dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
                }
            }
            world.setBlock(x, y, z, this.id, depth);
        }
    }

    private int getDistanceToGap(World world, int x, int y, int z, int distance, int fromDirection) {
        int n = 1000;
        for (int i = 0; i < 4; ++i) {
            int n2;
            if (i == 0 && fromDirection == 1 || i == 1 && fromDirection == 0 || i == 2 && fromDirection == 3 || i == 3 && fromDirection == 2) continue;
            int n3 = x;
            int n4 = y;
            int n5 = z;
            if (i == 0) {
                --n3;
            }
            if (i == 1) {
                ++n3;
            }
            if (i == 2) {
                --n5;
            }
            if (i == 3) {
                ++n5;
            }
            if (this.isLiquidBreaking(world, n3, n4, n5) || world.getMaterial(n3, n4, n5) == this.material && world.getBlockMeta(n3, n4, n5) == 0) continue;
            if (!this.isLiquidBreaking(world, n3, n4 - 1, n5)) {
                return distance;
            }
            if (distance >= 4 || (n2 = this.getDistanceToGap(world, n3, n4, n5, distance + 1, i)) >= n) continue;
            n = n2;
        }
        return n;
    }

    private boolean[] getSpread(World world, int x, int y, int z) {
        int n;
        int n2;
        for (n2 = 0; n2 < 4; ++n2) {
            this.distanceToGap[n2] = 1000;
            n = x;
            int n3 = y;
            int n4 = z;
            if (n2 == 0) {
                --n;
            }
            if (n2 == 1) {
                ++n;
            }
            if (n2 == 2) {
                --n4;
            }
            if (n2 == 3) {
                ++n4;
            }
            if (this.isLiquidBreaking(world, n, n3, n4) || world.getMaterial(n, n3, n4) == this.material && world.getBlockMeta(n, n3, n4) == 0) continue;
            this.distanceToGap[n2] = !this.isLiquidBreaking(world, n, n3 - 1, n4) ? 0 : this.getDistanceToGap(world, n, n3, n4, 1, n2);
        }
        n2 = this.distanceToGap[0];
        for (n = 1; n < 4; ++n) {
            if (this.distanceToGap[n] >= n2) continue;
            n2 = this.distanceToGap[n];
        }
        for (n = 0; n < 4; ++n) {
            this.spread[n] = this.distanceToGap[n] == n2;
        }
        return this.spread;
    }

    private boolean isLiquidBreaking(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y, z);
        if (n == Block.DOOR.id || n == Block.IRON_DOOR.id || n == Block.SIGN.id || n == Block.LADDER.id || n == Block.SUGAR_CANE.id) {
            return true;
        }
        if (n == 0) {
            return false;
        }
        Material material = Block.BLOCKS[n].material;
        return material.blocksMovement();
    }

    protected int getLowestDepth(World world, int x, int y, int z, int depth) {
        int n = this.getLiquidState(world, x, y, z);
        if (n < 0) {
            return depth;
        }
        if (n == 0) {
            ++this.adjacentSources;
        }
        if (n >= 8) {
            n = 0;
        }
        return depth < 0 || n < depth ? n : depth;
    }

    private boolean canSpreadTo(World world, int x, int y, int z) {
        Material material = world.getMaterial(x, y, z);
        if (material == this.material) {
            return false;
        }
        if (material == Material.LAVA) {
            return false;
        }
        return !this.isLiquidBreaking(world, x, y, z);
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
        if (world.getBlockId(x, y, z) == this.id) {
            world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
        }
    }
}

