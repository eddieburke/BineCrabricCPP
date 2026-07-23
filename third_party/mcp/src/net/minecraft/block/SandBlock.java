/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.world.World;

public class SandBlock
extends Block {
    public static boolean fallInstantly = false;

    public SandBlock(int id, int textureId) {
        super(id, textureId, Material.SAND);
    }

    public void onPlaced(World world, int x, int y, int z) {
        world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        this.processFall(world, x, y, z);
    }

    private void processFall(World world, int x, int y, int z) {
        int n = x;
        int n2 = y;
        int n3 = z;
        if (SandBlock.canFallThrough(world, n, n2 - 1, n3) && n2 >= 0) {
            int n4 = 32;
            if (fallInstantly || !world.isRegionLoaded(x - n4, y - n4, z - n4, x + n4, y + n4, z + n4)) {
                world.setBlock(x, y, z, 0);
                while (SandBlock.canFallThrough(world, x, y - 1, z) && y > 0) {
                    --y;
                }
                if (y > 0) {
                    world.setBlock(x, y, z, this.id);
                }
            } else {
                FallingBlockEntity fallingBlockEntity = new FallingBlockEntity(world, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, this.id);
                world.spawnEntity(fallingBlockEntity);
            }
        }
    }

    public int getTickRate() {
        return 3;
    }

    public static boolean canFallThrough(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y, z);
        if (n == 0) {
            return true;
        }
        if (n == Block.FIRE.id) {
            return true;
        }
        Material material = Block.BLOCKS[n].material;
        if (material == Material.WATER) {
            return true;
        }
        return material == Material.LAVA;
    }
}

