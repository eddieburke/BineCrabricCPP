/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class ClayOreFeature
extends Feature {
    private int clayBlockId;
    private int blockCount;

    public ClayOreFeature(int blockCount) {
        this.clayBlockId = Block.CLAY.id;
        this.blockCount = blockCount;
    }

    public boolean generate(World world, Random random, int x, int y, int z) {
        if (world.getMaterial(x, y, z) != Material.WATER) {
            return false;
        }
        float f = random.nextFloat() * (float)Math.PI;
        double d = (float)(x + 8) + MathHelper.sin(f) * (float)this.blockCount / 8.0f;
        double d2 = (float)(x + 8) - MathHelper.sin(f) * (float)this.blockCount / 8.0f;
        double d3 = (float)(z + 8) + MathHelper.cos(f) * (float)this.blockCount / 8.0f;
        double d4 = (float)(z + 8) - MathHelper.cos(f) * (float)this.blockCount / 8.0f;
        double d5 = y + random.nextInt(3) + 2;
        double d6 = y + random.nextInt(3) + 2;
        for (int i = 0; i <= this.blockCount; ++i) {
            double d7 = d + (d2 - d) * (double)i / (double)this.blockCount;
            double d8 = d5 + (d6 - d5) * (double)i / (double)this.blockCount;
            double d9 = d3 + (d4 - d3) * (double)i / (double)this.blockCount;
            double d10 = random.nextDouble() * (double)this.blockCount / 16.0;
            double d11 = (double)(MathHelper.sin((float)i * (float)Math.PI / (float)this.blockCount) + 1.0f) * d10 + 1.0;
            double d12 = (double)(MathHelper.sin((float)i * (float)Math.PI / (float)this.blockCount) + 1.0f) * d10 + 1.0;
            int n = MathHelper.floor(d7 - d11 / 2.0);
            int n2 = MathHelper.floor(d7 + d11 / 2.0);
            int n3 = MathHelper.floor(d8 - d12 / 2.0);
            int n4 = MathHelper.floor(d8 + d12 / 2.0);
            int n5 = MathHelper.floor(d9 - d11 / 2.0);
            int n6 = MathHelper.floor(d9 + d11 / 2.0);
            for (int j = n; j <= n2; ++j) {
                for (int k = n3; k <= n4; ++k) {
                    for (int i2 = n5; i2 <= n6; ++i2) {
                        int n7;
                        double d13 = ((double)j + 0.5 - d7) / (d11 / 2.0);
                        double d14 = ((double)k + 0.5 - d8) / (d12 / 2.0);
                        double d15 = ((double)i2 + 0.5 - d9) / (d11 / 2.0);
                        if (!(d13 * d13 + d14 * d14 + d15 * d15 < 1.0) || (n7 = world.getBlockId(j, k, i2)) != Block.SAND.id) continue;
                        world.setBlockWithoutNotifyingNeighbors(j, k, i2, this.clayBlockId);
                    }
                }
            }
        }
        return true;
    }
}

