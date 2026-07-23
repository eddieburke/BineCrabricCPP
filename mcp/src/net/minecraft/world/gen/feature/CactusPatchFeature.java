/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class CactusPatchFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        for (int i = 0; i < 10; ++i) {
            int n;
            int n2;
            int n3 = x + random.nextInt(8) - random.nextInt(8);
            if (!world.isAir(n3, n2 = y + random.nextInt(4) - random.nextInt(4), n = z + random.nextInt(8) - random.nextInt(8))) continue;
            int n4 = 1 + random.nextInt(random.nextInt(3) + 1);
            for (int j = 0; j < n4; ++j) {
                if (!Block.CACTUS.canGrow(world, n3, n2 + j, n)) continue;
                world.setBlockWithoutNotifyingNeighbors(n3, n2 + j, n, Block.CACTUS.id);
            }
        }
        return true;
    }
}

