/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class PineTreeFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6 = random.nextInt(5) + 7;
        int n7 = n6 - random.nextInt(2) - 3;
        int n8 = n6 - n7;
        int n9 = 1 + random.nextInt(n8 + 1);
        boolean bl = true;
        if (y < 1 || y + n6 + 1 > 128) {
            return false;
        }
        for (n5 = y; n5 <= y + 1 + n6 && bl; ++n5) {
            n4 = 1;
            n4 = n5 - y < n7 ? 0 : n9;
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
        n4 = 0;
        for (n3 = y + n6; n3 >= y + n7; --n3) {
            for (n2 = x - n4; n2 <= x + n4; ++n2) {
                n = n2 - x;
                for (int i = z - n4; i <= z + n4; ++i) {
                    int n10 = i - z;
                    if (Math.abs(n) == n4 && Math.abs(n10) == n4 && n4 > 0 || Block.BLOCKS_OPAQUE[world.getBlockId(n2, n3, i)]) continue;
                    world.setBlockWithoutNotifyingNeighbors(n2, n3, i, Block.LEAVES.id, 1);
                }
            }
            if (n4 >= 1 && n3 == y + n7 + 1) {
                --n4;
                continue;
            }
            if (n4 >= n9) continue;
            ++n4;
        }
        for (n3 = 0; n3 < n6 - 1; ++n3) {
            n2 = world.getBlockId(x, y + n3, z);
            if (n2 != 0 && n2 != Block.LEAVES.id) continue;
            world.setBlockWithoutNotifyingNeighbors(x, y + n3, z, Block.LOG.id, 1);
        }
        return true;
    }
}

