/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class PumpkinPatchFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        for (int i = 0; i < 64; ++i) {
            int n;
            int n2;
            int n3 = x + random.nextInt(8) - random.nextInt(8);
            if (!world.isAir(n3, n2 = y + random.nextInt(4) - random.nextInt(4), n = z + random.nextInt(8) - random.nextInt(8)) || world.getBlockId(n3, n2 - 1, n) != Block.GRASS_BLOCK.id || !Block.PUMPKIN.canPlaceAt(world, n3, n2, n)) continue;
            world.setBlockWithoutNotifyingNeighbors(n3, n2, n, Block.PUMPKIN.id, random.nextInt(4));
        }
        return true;
    }
}

