/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Random;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.item.Item;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Facings;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class RedstoneWireBlock
extends Block {
    private boolean powered = true;
    private Set neighborsToUpdate = new HashSet();

    public RedstoneWireBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON_BREAKABLE);
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    }

    public int getTexture(int side, int meta) {
        return this.textureId;
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
        return 5;
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        return 0x800000;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return world.shouldSuffocate(x, y - 1, z);
    }

    private void updatePower(World world, int x, int y, int z) {
        this.doUpdatePower(world, x, y, z, x, y, z);
        ArrayList arrayList = new ArrayList(this.neighborsToUpdate);
        this.neighborsToUpdate.clear();
        for (int i = 0; i < arrayList.size(); ++i) {
            BlockPos blockPos = (BlockPos)arrayList.get(i);
            world.notifyNeighbors(blockPos.x, blockPos.y, blockPos.z, this.id);
        }
    }

    private void doUpdatePower(World world, int x, int y, int z, int sourceX, int sourceY, int sourceZ) {
        int n;
        int n2;
        int n3;
        int n4 = world.getBlockMeta(x, y, z);
        int n5 = 0;
        this.powered = false;
        boolean bl = world.isEmittingRedstonePower(x, y, z);
        this.powered = true;
        if (bl) {
            n5 = 15;
        } else {
            for (n3 = 0; n3 < 4; ++n3) {
                n2 = x;
                n = z;
                if (n3 == 0) {
                    --n2;
                }
                if (n3 == 1) {
                    ++n2;
                }
                if (n3 == 2) {
                    --n;
                }
                if (n3 == 3) {
                    ++n;
                }
                if (n2 != sourceX || y != sourceY || n != sourceZ) {
                    n5 = this.getHighestPowerWire(world, n2, y, n, n5);
                }
                if (world.shouldSuffocate(n2, y, n) && !world.shouldSuffocate(x, y + 1, z)) {
                    if (n2 == sourceX && y + 1 == sourceY && n == sourceZ) continue;
                    n5 = this.getHighestPowerWire(world, n2, y + 1, n, n5);
                    continue;
                }
                if (world.shouldSuffocate(n2, y, n) || n2 == sourceX && y - 1 == sourceY && n == sourceZ) continue;
                n5 = this.getHighestPowerWire(world, n2, y - 1, n, n5);
            }
            n5 = n5 > 0 ? --n5 : 0;
        }
        if (n4 != n5) {
            world.pauseTicking = true;
            world.setBlockMeta(x, y, z, n5);
            world.setBlocksDirty(x, y, z, x, y, z);
            world.pauseTicking = false;
            for (n3 = 0; n3 < 4; ++n3) {
                n2 = x;
                n = z;
                int n6 = y - 1;
                if (n3 == 0) {
                    --n2;
                }
                if (n3 == 1) {
                    ++n2;
                }
                if (n3 == 2) {
                    --n;
                }
                if (n3 == 3) {
                    ++n;
                }
                if (world.shouldSuffocate(n2, y, n)) {
                    n6 += 2;
                }
                int n7 = 0;
                n7 = this.getHighestPowerWire(world, n2, y, n, -1);
                n5 = world.getBlockMeta(x, y, z);
                if (n5 > 0) {
                    --n5;
                }
                if (n7 >= 0 && n7 != n5) {
                    this.doUpdatePower(world, n2, y, n, x, y, z);
                }
                n7 = this.getHighestPowerWire(world, n2, n6, n, -1);
                n5 = world.getBlockMeta(x, y, z);
                if (n5 > 0) {
                    --n5;
                }
                if (n7 < 0 || n7 == n5) continue;
                this.doUpdatePower(world, n2, n6, n, x, y, z);
            }
            if (n4 == 0 || n5 == 0) {
                this.neighborsToUpdate.add(new BlockPos(x, y, z));
                this.neighborsToUpdate.add(new BlockPos(x - 1, y, z));
                this.neighborsToUpdate.add(new BlockPos(x + 1, y, z));
                this.neighborsToUpdate.add(new BlockPos(x, y - 1, z));
                this.neighborsToUpdate.add(new BlockPos(x, y + 1, z));
                this.neighborsToUpdate.add(new BlockPos(x, y, z - 1));
                this.neighborsToUpdate.add(new BlockPos(x, y, z + 1));
            }
        }
    }

    private void updateNeighborsOfWire(World world, int x, int y, int z) {
        if (world.getBlockId(x, y, z) != this.id) {
            return;
        }
        world.notifyNeighbors(x, y, z, this.id);
        world.notifyNeighbors(x - 1, y, z, this.id);
        world.notifyNeighbors(x + 1, y, z, this.id);
        world.notifyNeighbors(x, y, z - 1, this.id);
        world.notifyNeighbors(x, y, z + 1, this.id);
        world.notifyNeighbors(x, y - 1, z, this.id);
        world.notifyNeighbors(x, y + 1, z, this.id);
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
        if (world.isRemote) {
            return;
        }
        this.updatePower(world, x, y, z);
        world.notifyNeighbors(x, y + 1, z, this.id);
        world.notifyNeighbors(x, y - 1, z, this.id);
        this.updateNeighborsOfWire(world, x - 1, y, z);
        this.updateNeighborsOfWire(world, x + 1, y, z);
        this.updateNeighborsOfWire(world, x, y, z - 1);
        this.updateNeighborsOfWire(world, x, y, z + 1);
        if (world.shouldSuffocate(x - 1, y, z)) {
            this.updateNeighborsOfWire(world, x - 1, y + 1, z);
        } else {
            this.updateNeighborsOfWire(world, x - 1, y - 1, z);
        }
        if (world.shouldSuffocate(x + 1, y, z)) {
            this.updateNeighborsOfWire(world, x + 1, y + 1, z);
        } else {
            this.updateNeighborsOfWire(world, x + 1, y - 1, z);
        }
        if (world.shouldSuffocate(x, y, z - 1)) {
            this.updateNeighborsOfWire(world, x, y + 1, z - 1);
        } else {
            this.updateNeighborsOfWire(world, x, y - 1, z - 1);
        }
        if (world.shouldSuffocate(x, y, z + 1)) {
            this.updateNeighborsOfWire(world, x, y + 1, z + 1);
        } else {
            this.updateNeighborsOfWire(world, x, y - 1, z + 1);
        }
    }

    public void onBreak(World world, int x, int y, int z) {
        super.onBreak(world, x, y, z);
        if (world.isRemote) {
            return;
        }
        world.notifyNeighbors(x, y + 1, z, this.id);
        world.notifyNeighbors(x, y - 1, z, this.id);
        this.updatePower(world, x, y, z);
        this.updateNeighborsOfWire(world, x - 1, y, z);
        this.updateNeighborsOfWire(world, x + 1, y, z);
        this.updateNeighborsOfWire(world, x, y, z - 1);
        this.updateNeighborsOfWire(world, x, y, z + 1);
        if (world.shouldSuffocate(x - 1, y, z)) {
            this.updateNeighborsOfWire(world, x - 1, y + 1, z);
        } else {
            this.updateNeighborsOfWire(world, x - 1, y - 1, z);
        }
        if (world.shouldSuffocate(x + 1, y, z)) {
            this.updateNeighborsOfWire(world, x + 1, y + 1, z);
        } else {
            this.updateNeighborsOfWire(world, x + 1, y - 1, z);
        }
        if (world.shouldSuffocate(x, y, z - 1)) {
            this.updateNeighborsOfWire(world, x, y + 1, z - 1);
        } else {
            this.updateNeighborsOfWire(world, x, y - 1, z - 1);
        }
        if (world.shouldSuffocate(x, y, z + 1)) {
            this.updateNeighborsOfWire(world, x, y + 1, z + 1);
        } else {
            this.updateNeighborsOfWire(world, x, y - 1, z + 1);
        }
    }

    private int getHighestPowerWire(World world, int x, int y, int z, int power) {
        if (world.getBlockId(x, y, z) != this.id) {
            return power;
        }
        int n = world.getBlockMeta(x, y, z);
        if (n > power) {
            return n;
        }
        return power;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        boolean bl = this.canPlaceAt(world, x, y, z);
        if (!bl) {
            this.dropStacks(world, x, y, z, n);
            world.setBlock(x, y, z, 0);
        } else {
            this.updatePower(world, x, y, z);
        }
        super.neighborUpdate(world, x, y, z, id);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.REDSTONE.id;
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        if (!this.powered) {
            return false;
        }
        return this.isEmittingRedstonePowerInDirection(world, x, y, z, direction);
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        boolean bl;
        if (!this.powered) {
            return false;
        }
        if (blockView.getBlockMeta(x, y, z) == 0) {
            return false;
        }
        if (direction == 1) {
            return true;
        }
        boolean bl2 = RedstoneWireBlock.shouldConnectTo(blockView, x - 1, y, z, 1) || !blockView.shouldSuffocate(x - 1, y, z) && RedstoneWireBlock.shouldConnectTo(blockView, x - 1, y - 1, z, -1);
        boolean bl3 = RedstoneWireBlock.shouldConnectTo(blockView, x + 1, y, z, 3) || !blockView.shouldSuffocate(x + 1, y, z) && RedstoneWireBlock.shouldConnectTo(blockView, x + 1, y - 1, z, -1);
        boolean bl4 = RedstoneWireBlock.shouldConnectTo(blockView, x, y, z - 1, 2) || !blockView.shouldSuffocate(x, y, z - 1) && RedstoneWireBlock.shouldConnectTo(blockView, x, y - 1, z - 1, -1);
        boolean bl5 = bl = RedstoneWireBlock.shouldConnectTo(blockView, x, y, z + 1, 0) || !blockView.shouldSuffocate(x, y, z + 1) && RedstoneWireBlock.shouldConnectTo(blockView, x, y - 1, z + 1, -1);
        if (!blockView.shouldSuffocate(x, y + 1, z)) {
            if (blockView.shouldSuffocate(x - 1, y, z) && RedstoneWireBlock.shouldConnectTo(blockView, x - 1, y + 1, z, -1)) {
                bl2 = true;
            }
            if (blockView.shouldSuffocate(x + 1, y, z) && RedstoneWireBlock.shouldConnectTo(blockView, x + 1, y + 1, z, -1)) {
                bl3 = true;
            }
            if (blockView.shouldSuffocate(x, y, z - 1) && RedstoneWireBlock.shouldConnectTo(blockView, x, y + 1, z - 1, -1)) {
                bl4 = true;
            }
            if (blockView.shouldSuffocate(x, y, z + 1) && RedstoneWireBlock.shouldConnectTo(blockView, x, y + 1, z + 1, -1)) {
                bl = true;
            }
        }
        if (!(bl4 || bl3 || bl2 || bl || direction < 2 || direction > 5)) {
            return true;
        }
        if (direction == 2 && bl4 && !bl2 && !bl3) {
            return true;
        }
        if (direction == 3 && bl && !bl2 && !bl3) {
            return true;
        }
        if (direction == 4 && bl2 && !bl4 && !bl) {
            return true;
        }
        return direction == 5 && bl3 && !bl4 && !bl;
    }

    public boolean canEmitRedstonePower() {
        return this.powered;
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        int n = world.getBlockMeta(x, y, z);
        if (n > 0) {
            double d = (double)x + 0.5 + ((double)random.nextFloat() - 0.5) * 0.2;
            double d2 = (float)y + 0.0625f;
            double d3 = (double)z + 0.5 + ((double)random.nextFloat() - 0.5) * 0.2;
            float f = (float)n / 15.0f;
            float f2 = f * 0.6f + 0.4f;
            if (n == 0) {
                f2 = 0.0f;
            }
            float f3 = f * f * 0.7f - 0.5f;
            float f4 = f * f * 0.6f - 0.7f;
            if (f3 < 0.0f) {
                f3 = 0.0f;
            }
            if (f4 < 0.0f) {
                f4 = 0.0f;
            }
            world.addParticle("reddust", d, d2, d3, f2, f3, f4);
        }
    }

    public static boolean shouldConnectTo(BlockView blockView, int x, int y, int z, int l) {
        int n = blockView.getBlockId(x, y, z);
        if (n == Block.REDSTONE_WIRE.id) {
            return true;
        }
        if (n == 0) {
            return false;
        }
        if (Block.BLOCKS[n].canEmitRedstonePower()) {
            return true;
        }
        if (n == Block.REPEATER.id || n == Block.POWERED_REPEATER.id) {
            int n2 = blockView.getBlockMeta(x, y, z);
            return l == Facings.OPPOSITE[n2 & 3];
        }
        return false;
    }
}

