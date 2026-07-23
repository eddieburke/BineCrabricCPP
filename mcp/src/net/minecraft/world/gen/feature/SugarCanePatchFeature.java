/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class SugarCanePatchFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        for (int i = 0; i < 20; ++i) {
            int n;
            int n2;
            int n3 = x + random.nextInt(4) - random.nextInt(4);
            if (!world.isAir(n3, n2 = y, n = z + random.nextInt(4) - random.nextInt(4)) || world.getMaterial(n3 - 1, n2 - 1, n) != Material.WATER && world.getMaterial(n3 + 1, n2 - 1, n) != Material.WATER && world.getMaterial(n3, n2 - 1, n - 1) != Material.WATER && world.getMaterial(n3, n2 - 1, n + 1) != Material.WATER) continue;
            int n4 = 2 + random.nextInt(random.nextInt(3) + 1);
            for (int j = 0; j < n4; ++j) {
                if (!Block.SUGAR_CANE.canGrow(world, n3, n2 + j, n)) continue;
                world.setBlockWithoutNotifyingNeighbors(n3, n2 + j, n, Block.SUGAR_CANE.id);
            }
        }
        return true;
    }
}

