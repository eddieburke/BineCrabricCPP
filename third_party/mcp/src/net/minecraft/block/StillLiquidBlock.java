/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.material.Material;
import net.minecraft.world.World;

public class StillLiquidBlock
extends LiquidBlock {
    protected StillLiquidBlock(int i, Material arg) {
        super(i, arg);
        this.setTickRandomly(false);
        if (arg == Material.LAVA) {
            this.setTickRandomly(true);
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        super.neighborUpdate(world, x, y, z, id);
        if (world.getBlockId(x, y, z) == this.id) {
            this.convertToFlowing(world, x, y, z);
        }
    }

    private void convertToFlowing(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        world.pauseTicking = true;
        world.setBlockWithoutNotifyingNeighbors(x, y, z, this.id - 1, n);
        world.setBlocksDirty(x, y, z, x, y, z);
        world.scheduleBlockUpdate(x, y, z, this.id - 1, this.getTickRate());
        world.pauseTicking = false;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (this.material == Material.LAVA) {
            int n = random.nextInt(3);
            for (int i = 0; i < n; ++i) {
                int n2 = world.getBlockId(x += random.nextInt(3) - 1, ++y, z += random.nextInt(3) - 1);
                if (n2 == 0) {
                    if (!this.isFlammable(world, x - 1, y, z) && !this.isFlammable(world, x + 1, y, z) && !this.isFlammable(world, x, y, z - 1) && !this.isFlammable(world, x, y, z + 1) && !this.isFlammable(world, x, y - 1, z) && !this.isFlammable(world, x, y + 1, z)) continue;
                    world.setBlock(x, y, z, Block.FIRE.id);
                    return;
                }
                if (!Block.BLOCKS[n2].material.blocksMovement()) continue;
                return;
            }
        }
    }

    private boolean isFlammable(World world, int x, int y, int z) {
        return world.getMaterial(x, y, z).isBurnable();
    }
}

