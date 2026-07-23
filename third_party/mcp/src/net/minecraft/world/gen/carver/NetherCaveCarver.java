/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.carver;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.gen.Generator;

public class NetherCaveCarver
extends Generator {
    protected void place(int chunkX, int chunkZ, byte[] blocks, double x, double y, double z) {
        this.placeTunnels(chunkX, chunkZ, blocks, x, y, z, 1.0f + this.random.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
    }

    protected void placeTunnels(int chunkX, int chunkZ, byte[] blocks, double x, double y, double z, float baseWidth, float yaw, float pitch, int tunnel, int tunnelCount, double widthHeightRatio) {
        boolean bl;
        int n;
        double d = chunkX * 16 + 8;
        double d2 = chunkZ * 16 + 8;
        float f = 0.0f;
        float f2 = 0.0f;
        Random random = new Random(this.random.nextLong());
        if (tunnelCount <= 0) {
            n = this.range * 16 - 16;
            tunnelCount = n - random.nextInt(n / 4);
        }
        n = 0;
        if (tunnel == -1) {
            tunnel = tunnelCount / 2;
            n = 1;
        }
        int n2 = random.nextInt(tunnelCount / 2) + tunnelCount / 4;
        boolean bl2 = bl = random.nextInt(6) == 0;
        while (tunnel < tunnelCount) {
            double d3 = 1.5 + (double)(MathHelper.sin((float)tunnel * (float)Math.PI / (float)tunnelCount) * baseWidth * 1.0f);
            double d4 = d3 * widthHeightRatio;
            float f3 = MathHelper.cos(pitch);
            float f4 = MathHelper.sin(pitch);
            x += (double)(MathHelper.cos(yaw) * f3);
            y += (double)f4;
            z += (double)(MathHelper.sin(yaw) * f3);
            pitch = bl ? (pitch *= 0.92f) : (pitch *= 0.7f);
            pitch += f2 * 0.1f;
            yaw += f * 0.1f;
            f2 *= 0.9f;
            f *= 0.75f;
            f2 += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 2.0f;
            f += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 4.0f;
            if (n == 0 && tunnel == n2 && baseWidth > 1.0f) {
                this.placeTunnels(chunkX, chunkZ, blocks, x, y, z, random.nextFloat() * 0.5f + 0.5f, yaw - 1.5707964f, pitch / 3.0f, tunnel, tunnelCount, 1.0);
                this.placeTunnels(chunkX, chunkZ, blocks, x, y, z, random.nextFloat() * 0.5f + 0.5f, yaw + 1.5707964f, pitch / 3.0f, tunnel, tunnelCount, 1.0);
                return;
            }
            if (n != 0 || random.nextInt(4) != 0) {
                double d5 = x - d;
                double d6 = z - d2;
                double d7 = tunnelCount - tunnel;
                double d8 = baseWidth + 2.0f + 16.0f;
                if (d5 * d5 + d6 * d6 - d7 * d7 > d8 * d8) {
                    return;
                }
                if (!(x < d - 16.0 - d3 * 2.0 || z < d2 - 16.0 - d3 * 2.0 || x > d + 16.0 + d3 * 2.0 || z > d2 + 16.0 + d3 * 2.0)) {
                    int n3;
                    int n4;
                    int n5 = MathHelper.floor(x - d3) - chunkX * 16 - 1;
                    int n6 = MathHelper.floor(x + d3) - chunkX * 16 + 1;
                    int n7 = MathHelper.floor(y - d4) - 1;
                    int n8 = MathHelper.floor(y + d4) + 1;
                    int n9 = MathHelper.floor(z - d3) - chunkZ * 16 - 1;
                    int n10 = MathHelper.floor(z + d3) - chunkZ * 16 + 1;
                    if (n5 < 0) {
                        n5 = 0;
                    }
                    if (n6 > 16) {
                        n6 = 16;
                    }
                    if (n7 < 1) {
                        n7 = 1;
                    }
                    if (n8 > 120) {
                        n8 = 120;
                    }
                    if (n9 < 0) {
                        n9 = 0;
                    }
                    if (n10 > 16) {
                        n10 = 16;
                    }
                    boolean bl3 = false;
                    for (n4 = n5; !bl3 && n4 < n6; ++n4) {
                        for (int i = n9; !bl3 && i < n10; ++i) {
                            for (int j = n8 + 1; !bl3 && j >= n7 - 1; --j) {
                                n3 = (n4 * 16 + i) * 128 + j;
                                if (j < 0 || j >= 128) continue;
                                if (blocks[n3] == Block.FLOWING_LAVA.id || blocks[n3] == Block.LAVA.id) {
                                    bl3 = true;
                                }
                                if (j == n7 - 1 || n4 == n5 || n4 == n6 - 1 || i == n9 || i == n10 - 1) continue;
                                j = n7;
                            }
                        }
                    }
                    if (!bl3) {
                        for (n4 = n5; n4 < n6; ++n4) {
                            double d9 = ((double)(n4 + chunkX * 16) + 0.5 - x) / d3;
                            for (n3 = n9; n3 < n10; ++n3) {
                                double d10 = ((double)(n3 + chunkZ * 16) + 0.5 - z) / d3;
                                int n11 = (n4 * 16 + n3) * 128 + n8;
                                for (int i = n8 - 1; i >= n7; --i) {
                                    byte by;
                                    double d11 = ((double)i + 0.5 - y) / d4;
                                    if (d11 > -0.7 && d9 * d9 + d11 * d11 + d10 * d10 < 1.0 && ((by = blocks[n11]) == Block.NETHERRACK.id || by == Block.DIRT.id || by == Block.GRASS_BLOCK.id)) {
                                        blocks[n11] = 0;
                                    }
                                    --n11;
                                }
                            }
                        }
                        if (n != 0) break;
                    }
                }
            }
            ++tunnel;
        }
    }

    protected void place(World world, int startChunkX, int startChunkZ, int chunkX, int chunkZ, byte[] blocks) {
        int n = this.random.nextInt(this.random.nextInt(this.random.nextInt(10) + 1) + 1);
        if (this.random.nextInt(5) != 0) {
            n = 0;
        }
        for (int i = 0; i < n; ++i) {
            double d = startChunkX * 16 + this.random.nextInt(16);
            double d2 = this.random.nextInt(128);
            double d3 = startChunkZ * 16 + this.random.nextInt(16);
            int n2 = 1;
            if (this.random.nextInt(4) == 0) {
                this.place(chunkX, chunkZ, blocks, d, d2, d3);
                n2 += this.random.nextInt(4);
            }
            for (int j = 0; j < n2; ++j) {
                float f = this.random.nextFloat() * (float)Math.PI * 2.0f;
                float f2 = (this.random.nextFloat() - 0.5f) * 2.0f / 8.0f;
                float f3 = this.random.nextFloat() * 2.0f + this.random.nextFloat();
                this.placeTunnels(chunkX, chunkZ, blocks, d, d2, d3, f3 * 2.0f, f, f2, 0, 0, 0.5);
            }
        }
    }
}

