/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class OakTreeFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6 = random.nextInt(3) + 4;
        boolean bl = true;
        if (y < 1 || y + n6 + 1 > 128) {
            return false;
        }
        for (n5 = y; n5 <= y + 1 + n6; ++n5) {
            n4 = 1;
            if (n5 == y) {
                n4 = 0;
            }
            if (n5 >= y + 1 + n6 - 2) {
                n4 = 2;
            }
            for (n3 = x - n4; n3 <= x + n4 && bl; ++n3) {
                for (n2 = z - n4; n2 <= z + n4 && bl; ++n2) {
                    if (n5 >= 0 && n5 < 128) {
                        n = world.getBlockId(n3, n5, n2);
                        if (n == 0 || n == Block.LEAVES.id) continue;
                        bl = false;
                        continue;
                    }
                    bl = false;
                }
            }
        }
        if (!bl) {
            return false;
        }
        n5 = world.getBlockId(x, y - 1, z);
        if (n5 != Block.GRASS_BLOCK.id && n5 != Block.DIRT.id || y >= 128 - n6 - 1) {
            return false;
        }
        world.setBlockWithoutNotifyingNeighbors(x, y - 1, z, Block.DIRT.id);
        for (n4 = y - 3 + n6; n4 <= y + n6; ++n4) {
            n3 = n4 - (y + n6);
            n2 = 1 - n3 / 2;
            for (n = x - n2; n <= x + n2; ++n) {
                int n7 = n - x;
                for (int i = z - n2; i <= z + n2; ++i) {
                    int n8 = i - z;
                    if (Math.abs(n7) == n2 && Math.abs(n8) == n2 && (random.nextInt(2) == 0 || n3 == 0) || Block.BLOCKS_OPAQUE[world.getBlockId(n, n4, i)]) continue;
                    world.setBlockWithoutNotifyingNeighbors(n, n4, i, Block.LEAVES.id);
                }
            }
        }
        for (n4 = 0; n4 < n6; ++n4) {
            n3 = world.getBlockId(x, y + n4, z);
            if (n3 != 0 && n3 != Block.LEAVES.id) continue;
            world.setBlockWithoutNotifyingNeighbors(x, y + n4, z, Block.LOG.id);
        }
        return true;
    }
}

