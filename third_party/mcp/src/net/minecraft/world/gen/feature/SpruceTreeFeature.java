/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class SpruceTreeFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        int n7;
        int n8 = random.nextInt(4) + 6;
        int n9 = 1 + random.nextInt(2);
        int n10 = n8 - n9;
        int n11 = 2 + random.nextInt(2);
        boolean bl = true;
        if (y < 1 || y + n8 + 1 > 128) {
            return false;
        }
        for (n7 = y; n7 <= y + 1 + n8 && bl; ++n7) {
            n6 = 1;
            n6 = n7 - y < n9 ? 0 : n11;
            for (n5 = x - n6; n5 <= x + n6 && bl; ++n5) {
                for (n4 = z - n6; n4 <= z + n6 && bl; ++n4) {
                    if (n7 >= 0 && n7 < 128) {
                        n3 = world.getBlockId(n5, n7, n4);
                        if (n3 == 0 || n3 == Block.LEAVES.id) continue;
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
        n7 = world.getBlockId(x, y - 1, z);
        if (n7 != Block.GRASS_BLOCK.id && n7 != Block.DIRT.id || y >= 128 - n8 - 1) {
            return false;
        }
        world.setBlockWithoutNotifyingNeighbors(x, y - 1, z, Block.DIRT.id);
        n6 = random.nextInt(2);
        n5 = 1;
        n4 = 0;
        for (n3 = 0; n3 <= n10; ++n3) {
            n2 = y + n8 - n3;
            for (n = x - n6; n <= x + n6; ++n) {
                int n12 = n - x;
                for (int i = z - n6; i <= z + n6; ++i) {
                    int n13 = i - z;
                    if (Math.abs(n12) == n6 && Math.abs(n13) == n6 && n6 > 0 || Block.BLOCKS_OPAQUE[world.getBlockId(n, n2, i)]) continue;
                    world.setBlockWithoutNotifyingNeighbors(n, n2, i, Block.LEAVES.id, 1);
                }
            }
            if (n6 >= n5) {
                n6 = n4;
                n4 = 1;
                if (++n5 <= n11) continue;
                n5 = n11;
                continue;
            }
            ++n6;
        }
        n3 = random.nextInt(3);
        for (n2 = 0; n2 < n8 - n3; ++n2) {
            n = world.getBlockId(x, y + n2, z);
            if (n != 0 && n != Block.LEAVES.id) continue;
            world.setBlockWithoutNotifyingNeighbors(x, y + n2, z, Block.LOG.id, 1);
        }
        return true;
    }
}

