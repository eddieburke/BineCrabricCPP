/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.gen.chunk;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.SandBlock;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.util.math.noise.OctavePerlinNoiseSampler;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.gen.Generator;
import net.minecraft.world.gen.carver.NetherCaveCarver;
import net.minecraft.world.gen.feature.GlowstoneClusterFeature;
import net.minecraft.world.gen.feature.GlowstoneClusterRareFeature;
import net.minecraft.world.gen.feature.NetherFirePatchFeature;
import net.minecraft.world.gen.feature.NetherLavaSpringFeature;
import net.minecraft.world.gen.feature.PlantPatchFeature;

public class NetherChunkGenerator
implements ChunkSource {
    private Random random;
    private OctavePerlinNoiseSampler minLimitPerlinNoise;
    private OctavePerlinNoiseSampler maxLimitPerlinNoise;
    private OctavePerlinNoiseSampler perlinNoise1;
    private OctavePerlinNoiseSampler perlinNoise2;
    private OctavePerlinNoiseSampler perlinNoise3;
    public OctavePerlinNoiseSampler scaleNoise;
    public OctavePerlinNoiseSampler depthNoise;
    private World world;
    private double[] heightMap;
    private double[] sandBuffer = new double[256];
    private double[] gravelBuffer = new double[256];
    private double[] depthBuffer = new double[256];
    private Generator cave = new NetherCaveCarver();
    double[] perlinNoiseBuffer;
    double[] minLimitPerlinNoiseBuffer;
    double[] maxLimitPerlinNoiseBuffer;
    double[] scaleNoiseBuffer;
    double[] depthNoiseBuffer;

    public NetherChunkGenerator(World world, long seed) {
        this.world = world;
        this.random = new Random(seed);
        this.minLimitPerlinNoise = new OctavePerlinNoiseSampler(this.random, 16);
        this.maxLimitPerlinNoise = new OctavePerlinNoiseSampler(this.random, 16);
        this.perlinNoise1 = new OctavePerlinNoiseSampler(this.random, 8);
        this.perlinNoise2 = new OctavePerlinNoiseSampler(this.random, 4);
        this.perlinNoise3 = new OctavePerlinNoiseSampler(this.random, 4);
        this.scaleNoise = new OctavePerlinNoiseSampler(this.random, 10);
        this.depthNoise = new OctavePerlinNoiseSampler(this.random, 16);
    }

    public void buildTerrain(int chunkX, int chunkZ, byte[] blocks) {
        int n = 4;
        int n2 = 32;
        int n3 = n + 1;
        int n4 = 17;
        int n5 = n + 1;
        this.heightMap = this.generateHeightMap(this.heightMap, chunkX * n, 0, chunkZ * n, n3, n4, n5);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < 16; ++k) {
                    double d = 0.125;
                    double d2 = this.heightMap[((i + 0) * n5 + (j + 0)) * n4 + (k + 0)];
                    double d3 = this.heightMap[((i + 0) * n5 + (j + 1)) * n4 + (k + 0)];
                    double d4 = this.heightMap[((i + 1) * n5 + (j + 0)) * n4 + (k + 0)];
                    double d5 = this.heightMap[((i + 1) * n5 + (j + 1)) * n4 + (k + 0)];
                    double d6 = (this.heightMap[((i + 0) * n5 + (j + 0)) * n4 + (k + 1)] - d2) * d;
                    double d7 = (this.heightMap[((i + 0) * n5 + (j + 1)) * n4 + (k + 1)] - d3) * d;
                    double d8 = (this.heightMap[((i + 1) * n5 + (j + 0)) * n4 + (k + 1)] - d4) * d;
                    double d9 = (this.heightMap[((i + 1) * n5 + (j + 1)) * n4 + (k + 1)] - d5) * d;
                    for (int i2 = 0; i2 < 8; ++i2) {
                        double d10 = 0.25;
                        double d11 = d2;
                        double d12 = d3;
                        double d13 = (d4 - d2) * d10;
                        double d14 = (d5 - d3) * d10;
                        for (int i3 = 0; i3 < 4; ++i3) {
                            int n6 = i3 + i * 4 << 11 | 0 + j * 4 << 7 | k * 8 + i2;
                            int n7 = 128;
                            double d15 = 0.25;
                            double d16 = d11;
                            double d17 = (d12 - d11) * d15;
                            for (int i4 = 0; i4 < 4; ++i4) {
                                int n8 = 0;
                                if (k * 8 + i2 < n2) {
                                    n8 = Block.LAVA.id;
                                }
                                if (d16 > 0.0) {
                                    n8 = Block.NETHERRACK.id;
                                }
                                blocks[n6] = (byte)n8;
                                n6 += n7;
                                d16 += d17;
                            }
                            d11 += d13;
                            d12 += d14;
                        }
                        d2 += d6;
                        d3 += d7;
                        d4 += d8;
                        d5 += d9;
                    }
                }
            }
        }
    }

    public void buildSurfaces(int chunkX, int chunkY, byte[] blocks) {
        int n = 64;
        double d = 0.03125;
        this.sandBuffer = this.perlinNoise2.create(this.sandBuffer, chunkX * 16, chunkY * 16, 0.0, 16, 16, 1, d, d, 1.0);
        this.gravelBuffer = this.perlinNoise2.create(this.gravelBuffer, chunkX * 16, 109.0134, chunkY * 16, 16, 1, 16, d, 1.0, d);
        this.depthBuffer = this.perlinNoise3.create(this.depthBuffer, chunkX * 16, chunkY * 16, 0.0, 16, 16, 1, d * 2.0, d * 2.0, d * 2.0);
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                boolean bl = this.sandBuffer[i + j * 16] + this.random.nextDouble() * 0.2 > 0.0;
                boolean bl2 = this.gravelBuffer[i + j * 16] + this.random.nextDouble() * 0.2 > 0.0;
                int n2 = (int)(this.depthBuffer[i + j * 16] / 3.0 + 3.0 + this.random.nextDouble() * 0.25);
                int n3 = -1;
                byte by = (byte)Block.NETHERRACK.id;
                byte by2 = (byte)Block.NETHERRACK.id;
                for (int k = 127; k >= 0; --k) {
                    int n4 = (j * 16 + i) * 128 + k;
                    if (k >= 127 - this.random.nextInt(5)) {
                        blocks[n4] = (byte)Block.BEDROCK.id;
                        continue;
                    }
                    if (k <= 0 + this.random.nextInt(5)) {
                        blocks[n4] = (byte)Block.BEDROCK.id;
                        continue;
                    }
                    byte by3 = blocks[n4];
                    if (by3 == 0) {
                        n3 = -1;
                        continue;
                    }
                    if (by3 != Block.NETHERRACK.id) continue;
                    if (n3 == -1) {
                        if (n2 <= 0) {
                            by = 0;
                            by2 = (byte)Block.NETHERRACK.id;
                        } else if (k >= n - 4 && k <= n + 1) {
                            by = (byte)Block.NETHERRACK.id;
                            by2 = (byte)Block.NETHERRACK.id;
                            if (bl2) {
                                by = (byte)Block.GRAVEL.id;
                            }
                            if (bl2) {
                                by2 = (byte)Block.NETHERRACK.id;
                            }
                            if (bl) {
                                by = (byte)Block.SOUL_SAND.id;
                            }
                            if (bl) {
                                by2 = (byte)Block.SOUL_SAND.id;
                            }
                        }
                        if (k < n && by == 0) {
                            by = (byte)Block.LAVA.id;
                        }
                        n3 = n2;
                        if (k >= n - 1) {
                            blocks[n4] = by;
                            continue;
                        }
                        blocks[n4] = by2;
                        continue;
                    }
                    if (n3 <= 0) continue;
                    --n3;
                    blocks[n4] = by2;
                }
            }
        }
    }

    public Chunk loadChunk(int chunkX, int chunkZ) {
        return this.getChunk(chunkX, chunkZ);
    }

    public Chunk getChunk(int chunkX, int chunkZ) {
        this.random.setSeed((long)chunkX * 341873128712L + (long)chunkZ * 132897987541L);
        byte[] byArray = new byte[32768];
        this.buildTerrain(chunkX, chunkZ, byArray);
        this.buildSurfaces(chunkX, chunkZ, byArray);
        this.cave.place(this, this.world, chunkX, chunkZ, byArray);
        Chunk chunk = new Chunk(this.world, byArray, chunkX, chunkZ);
        return chunk;
    }

    private double[] generateHeightMap(double[] heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
        int n;
        if (heightMap == null) {
            heightMap = new double[sizeX * sizeY * sizeZ];
        }
        double d = 684.412;
        double d2 = 2053.236;
        this.scaleNoiseBuffer = this.scaleNoise.create(this.scaleNoiseBuffer, x, y, z, sizeX, 1, sizeZ, 1.0, 0.0, 1.0);
        this.depthNoiseBuffer = this.depthNoise.create(this.depthNoiseBuffer, x, y, z, sizeX, 1, sizeZ, 100.0, 0.0, 100.0);
        this.perlinNoiseBuffer = this.perlinNoise1.create(this.perlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d / 80.0, d2 / 60.0, d / 80.0);
        this.minLimitPerlinNoiseBuffer = this.minLimitPerlinNoise.create(this.minLimitPerlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d, d2, d);
        this.maxLimitPerlinNoiseBuffer = this.maxLimitPerlinNoise.create(this.maxLimitPerlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d, d2, d);
        int n2 = 0;
        int n3 = 0;
        double[] dArray = new double[sizeY];
        for (n = 0; n < sizeY; ++n) {
            dArray[n] = Math.cos((double)n * Math.PI * 6.0 / (double)sizeY) * 2.0;
            double d3 = n;
            if (n > sizeY / 2) {
                d3 = sizeY - 1 - n;
            }
            if (!(d3 < 4.0)) continue;
            d3 = 4.0 - d3;
            int n4 = n;
            dArray[n4] = dArray[n4] - d3 * d3 * d3 * 10.0;
        }
        for (n = 0; n < sizeX; ++n) {
            for (int i = 0; i < sizeZ; ++i) {
                double d4 = (this.scaleNoiseBuffer[n3] + 256.0) / 512.0;
                if (d4 > 1.0) {
                    d4 = 1.0;
                }
                double d5 = 0.0;
                double d6 = this.depthNoiseBuffer[n3] / 8000.0;
                if (d6 < 0.0) {
                    d6 = -d6;
                }
                if ((d6 = d6 * 3.0 - 3.0) < 0.0) {
                    if ((d6 /= 2.0) < -1.0) {
                        d6 = -1.0;
                    }
                    d6 /= 1.4;
                    d6 /= 2.0;
                    d4 = 0.0;
                } else {
                    if (d6 > 1.0) {
                        d6 = 1.0;
                    }
                    d6 /= 6.0;
                }
                d4 += 0.5;
                d6 = d6 * (double)sizeY / 16.0;
                ++n3;
                for (int j = 0; j < sizeY; ++j) {
                    double d7;
                    double d8 = 0.0;
                    double d9 = dArray[j];
                    double d10 = this.minLimitPerlinNoiseBuffer[n2] / 512.0;
                    double d11 = this.maxLimitPerlinNoiseBuffer[n2] / 512.0;
                    double d12 = (this.perlinNoiseBuffer[n2] / 10.0 + 1.0) / 2.0;
                    d8 = d12 < 0.0 ? d10 : (d12 > 1.0 ? d11 : d10 + (d11 - d10) * d12);
                    d8 -= d9;
                    if (j > sizeY - 4) {
                        d7 = (float)(j - (sizeY - 4)) / 3.0f;
                        d8 = d8 * (1.0 - d7) + -10.0 * d7;
                    }
                    if ((double)j < d5) {
                        d7 = (d5 - (double)j) / 4.0;
                        if (d7 < 0.0) {
                            d7 = 0.0;
                        }
                        if (d7 > 1.0) {
                            d7 = 1.0;
                        }
                        d8 = d8 * (1.0 - d7) + -10.0 * d7;
                    }
                    heightMap[n2] = d8;
                    ++n2;
                }
            }
        }
        return heightMap;
    }

    public boolean isChunkLoaded(int x, int z) {
        return true;
    }

    public void decorate(ChunkSource source, int x, int z) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        SandBlock.fallInstantly = true;
        int n6 = x * 16;
        int n7 = z * 16;
        for (n5 = 0; n5 < 8; ++n5) {
            n4 = n6 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(120) + 4;
            n2 = n7 + this.random.nextInt(16) + 8;
            new NetherLavaSpringFeature(Block.FLOWING_LAVA.id).generate(this.world, this.random, n4, n3, n2);
        }
        n5 = this.random.nextInt(this.random.nextInt(10) + 1) + 1;
        for (n4 = 0; n4 < n5; ++n4) {
            n3 = n6 + this.random.nextInt(16) + 8;
            n2 = this.random.nextInt(120) + 4;
            n = n7 + this.random.nextInt(16) + 8;
            new NetherFirePatchFeature().generate(this.world, this.random, n3, n2, n);
        }
        n5 = this.random.nextInt(this.random.nextInt(10) + 1);
        for (n4 = 0; n4 < n5; ++n4) {
            n3 = n6 + this.random.nextInt(16) + 8;
            n2 = this.random.nextInt(120) + 4;
            n = n7 + this.random.nextInt(16) + 8;
            new GlowstoneClusterFeature().generate(this.world, this.random, n3, n2, n);
        }
        for (n4 = 0; n4 < 10; ++n4) {
            n3 = n6 + this.random.nextInt(16) + 8;
            n2 = this.random.nextInt(128);
            n = n7 + this.random.nextInt(16) + 8;
            new GlowstoneClusterRareFeature().generate(this.world, this.random, n3, n2, n);
        }
        if (this.random.nextInt(1) == 0) {
            n4 = n6 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n7 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.BROWN_MUSHROOM.id).generate(this.world, this.random, n4, n3, n2);
        }
        if (this.random.nextInt(1) == 0) {
            n4 = n6 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n7 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.RED_MUSHROOM.id).generate(this.world, this.random, n4, n3, n2);
        }
        SandBlock.fallInstantly = false;
    }

    public boolean save(boolean saveEntities, LoadingDisplay display) {
        return true;
    }

    public boolean tick() {
        return false;
    }

    public boolean canSave() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public String getDebugInfo() {
        return "HellRandomLevelSource";
    }
}

