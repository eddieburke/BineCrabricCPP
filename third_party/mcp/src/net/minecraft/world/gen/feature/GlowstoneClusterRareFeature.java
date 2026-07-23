/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class GlowstoneClusterRareFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        if (!world.isAir(x, y, z)) {
            return false;
        }
        if (world.getBlockId(x, y + 1, z) != Block.NETHERRACK.id) {
            return false;
        }
        world.setBlock(x, y, z, Block.GLOWSTONE.id);
        for (int i = 0; i < 1500; ++i) {
            int n;
            int n2;
            int n3 = x + random.nextInt(8) - random.nextInt(8);
            if (world.getBlockId(n3, n2 = y - random.nextInt(12), n = z + random.nextInt(8) - random.nextInt(8)) != 0) continue;
            int n4 = 0;
            for (int j = 0; j < 6; ++j) {
                int n5 = 0;
                if (j == 0) {
                    n5 = world.getBlockId(n3 - 1, n2, n);
                }
                if (j == 1) {
                    n5 = world.getBlockId(n3 + 1, n2, n);
                }
                if (j == 2) {
                    n5 = world.getBlockId(n3, n2 - 1, n);
                }
                if (j == 3) {
                    n5 = world.getBlockId(n3, n2 + 1, n);
                }
                if (j == 4) {
                    n5 = world.getBlockId(n3, n2, n - 1);
                }
                if (j == 5) {
                    n5 = world.getBlockId(n3, n2, n + 1);
                }
                if (n5 != Block.GLOWSTONE.id) continue;
                ++n4;
            }
            if (n4 != true) continue;
            world.setBlock(n3, n2, n, Block.GLOWSTONE.id);
        }
        return true;
    }
}

