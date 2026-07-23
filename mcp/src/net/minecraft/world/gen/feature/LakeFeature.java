/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.world.LightType;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class LakeFeature
extends Feature {
    private int waterBlockId;

    public LakeFeature(int waterBlockId) {
        this.waterBlockId = waterBlockId;
    }

    public boolean generate(World world, Random random, int x, int y, int z) {
        int n;
        int n2;
        x -= 8;
        z -= 8;
        while (y > 0 && world.isAir(x, y, z)) {
            --y;
        }
        y -= 4;
        boolean[] blArray = new boolean[2048];
        int n3 = random.nextInt(4) + 4;
        for (n2 = 0; n2 < n3; ++n2) {
            double d = random.nextDouble() * 6.0 + 3.0;
            double d2 = random.nextDouble() * 4.0 + 2.0;
            double d3 = random.nextDouble() * 6.0 + 3.0;
            double d4 = random.nextDouble() * (16.0 - d - 2.0) + 1.0 + d / 2.0;
            double d5 = random.nextDouble() * (8.0 - d2 - 4.0) + 2.0 + d2 / 2.0;
            double d6 = random.nextDouble() * (16.0 - d3 - 2.0) + 1.0 + d3 / 2.0;
            for (int i = 1; i < 15; ++i) {
                for (int j = 1; j < 15; ++j) {
                    for (int k = 1; k < 7; ++k) {
                        double d7 = ((double)i - d4) / (d / 2.0);
                        double d8 = ((double)k - d5) / (d2 / 2.0);
                        double d9 = ((double)j - d6) / (d3 / 2.0);
                        double d10 = d7 * d7 + d8 * d8 + d9 * d9;
                        if (!(d10 < 1.0)) continue;
                        blArray[(i * 16 + j) * 8 + k] = true;
                    }
                }
            }
        }
        for (n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (n = 0; n < 8; ++n) {
                    boolean bl;
                    boolean bl2 = bl = !blArray[(n2 * 16 + i) * 8 + n] && (n2 < 15 && blArray[((n2 + 1) * 16 + i) * 8 + n] || n2 > 0 && blArray[((n2 - 1) * 16 + i) * 8 + n] || i < 15 && blArray[(n2 * 16 + (i + 1)) * 8 + n] || i > 0 && blArray[(n2 * 16 + (i - 1)) * 8 + n] || n < 7 && blArray[(n2 * 16 + i) * 8 + (n + 1)] || n > 0 && blArray[(n2 * 16 + i) * 8 + (n - 1)]);
                    if (!bl) continue;
                    Material material = world.getMaterial(x + n2, y + n, z + i);
                    if (n >= 4 && material.isFluid()) {
                        return false;
                    }
                    if (n >= 4 || material.isSolid() || world.getBlockId(x + n2, y + n, z + i) == this.waterBlockId) continue;
                    return false;
                }
            }
        }
        for (n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (n = 0; n < 8; ++n) {
                    if (!blArray[(n2 * 16 + i) * 8 + n]) continue;
                    world.setBlockWithoutNotifyingNeighbors(x + n2, y + n, z + i, n >= 4 ? 0 : this.waterBlockId);
                }
            }
        }
        for (n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (n = 4; n < 8; ++n) {
                    if (!blArray[(n2 * 16 + i) * 8 + n] || world.getBlockId(x + n2, y + n - 1, z + i) != Block.DIRT.id || world.getBrightness(LightType.SKY, x + n2, y + n, z + i) <= 0) continue;
                    world.setBlockWithoutNotifyingNeighbors(x + n2, y + n - 1, z + i, Block.GRASS_BLOCK.id);
                }
            }
        }
        if (Block.BLOCKS[this.waterBlockId].material == Material.LAVA) {
            for (n2 = 0; n2 < 16; ++n2) {
                for (int i = 0; i < 16; ++i) {
                    for (n = 0; n < 8; ++n) {
                        boolean bl;
                        boolean bl3 = bl = !blArray[(n2 * 16 + i) * 8 + n] && (n2 < 15 && blArray[((n2 + 1) * 16 + i) * 8 + n] || n2 > 0 && blArray[((n2 - 1) * 16 + i) * 8 + n] || i < 15 && blArray[(n2 * 16 + (i + 1)) * 8 + n] || i > 0 && blArray[(n2 * 16 + (i - 1)) * 8 + n] || n < 7 && blArray[(n2 * 16 + i) * 8 + (n + 1)] || n > 0 && blArray[(n2 * 16 + i) * 8 + (n - 1)]);
                        if (!bl || n >= 4 && random.nextInt(2) == 0 || !world.getMaterial(x + n2, y + n, z + i).isSolid()) continue;
                        world.setBlockWithoutNotifyingNeighbors(x + n2, y + n, z + i, Block.STONE.id);
                    }
                }
            }
        }
        return true;
    }
}

