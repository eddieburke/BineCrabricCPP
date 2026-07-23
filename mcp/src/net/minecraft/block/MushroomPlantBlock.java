/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.PlantBlock;
import net.minecraft.world.World;

public class MushroomPlantBlock
extends PlantBlock {
    protected MushroomPlantBlock(int i, int j) {
        super(i, j);
        float f = 0.2f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
        this.setTickRandomly(true);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        int n;
        int n2;
        int n3;
        if (random.nextInt(100) == 0 && world.isAir(n3 = x + random.nextInt(3) - 1, n2 = y + random.nextInt(2) - random.nextInt(2), n = z + random.nextInt(3) - 1) && this.canGrow(world, n3, n2, n)) {
            x += random.nextInt(3) - 1;
            z += random.nextInt(3) - 1;
            if (world.isAir(n3, n2, n) && this.canGrow(world, n3, n2, n)) {
                world.setBlock(n3, n2, n, this.id);
            }
        }
    }

    protected boolean canPlantOnTop(int id) {
        return Block.BLOCKS_OPAQUE[id];
    }

    public boolean canGrow(World world, int x, int y, int z) {
        if (y < 0 || y >= 128) {
            return false;
        }
        return world.getBrightness(x, y, z) < 13 && this.canPlantOnTop(world.getBlockId(x, y - 1, z));
    }
}

