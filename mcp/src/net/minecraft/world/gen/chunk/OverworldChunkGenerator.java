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
import net.minecraft.block.material.Material;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.util.math.noise.OctavePerlinNoiseSampler;
import net.minecraft.world.World;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.gen.Generator;
import net.minecraft.world.gen.carver.CaveWorldCarver;
import net.minecraft.world.gen.feature.CactusPatchFeature;
import net.minecraft.world.gen.feature.ClayOreFeature;
import net.minecraft.world.gen.feature.DeadBushPatchFeature;
import net.minecraft.world.gen.feature.DungeonFeature;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.GrassPatchFeature;
import net.minecraft.world.gen.feature.LakeFeature;
import net.minecraft.world.gen.feature.OreFeature;
import net.minecraft.world.gen.feature.PlantPatchFeature;
import net.minecraft.world.gen.feature.PumpkinPatchFeature;
import net.minecraft.world.gen.feature.SpringFeature;
import net.minecraft.world.gen.feature.SugarCanePatchFeature;

public class OverworldChunkGenerator
implements ChunkSource {
    private Random random;
    private OctavePerlinNoiseSampler minLimitPerlinNoise;
    private OctavePerlinNoiseSampler maxLimitPerlinNoise;
    private OctavePerlinNoiseSampler perlinNoise1;
    private OctavePerlinNoiseSampler perlinNoise2;
    private OctavePerlinNoiseSampler perlinNoise3;
    public OctavePerlinNoiseSampler floatingIslandScale;
    public OctavePerlinNoiseSampler floatingIslandNoise;
    public OctavePerlinNoiseSampler forestNoise;
    private World world;
    private double[] heightMap;
    private double[] sandBuffer = new double[256];
    private double[] gravelBuffer = new double[256];
    private double[] depthBuffer = new double[256];
    private Generator cave = new CaveWorldCarver();
    private Biome[] biomes;
    double[] perlinNoiseBuffer;
    double[] minLimitPerlinNoiseBuffer;
    double[] maxLimitPerlinNoiseBuffer;
    double[] scaleNoiseBuffer;
    double[] depthNoiseBuffer;
    int[][] waterDepths = new int[32][32];
    private double[] temperatures;

    public OverworldChunkGenerator(World world, long seed) {
        this.world = world;
        this.random = new Random(seed);
        this.minLimitPerlinNoise = new OctavePerlinNoiseSampler(this.random, 16);
        this.maxLimitPerlinNoise = new OctavePerlinNoiseSampler(this.random, 16);
        this.perlinNoise1 = new OctavePerlinNoiseSampler(this.random, 8);
        this.perlinNoise2 = new OctavePerlinNoiseSampler(this.random, 4);
        this.perlinNoise3 = new OctavePerlinNoiseSampler(this.random, 4);
        this.floatingIslandScale = new OctavePerlinNoiseSampler(this.random, 10);
        this.floatingIslandNoise = new OctavePerlinNoiseSampler(this.random, 16);
        this.forestNoise = new OctavePerlinNoiseSampler(this.random, 8);
    }

    public void buildTerrain(int chunkX, int chunkZ, byte[] blocks, Biome[] biomes, double[] temperatures) {
        int n = 4;
        int n2 = 64;
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
                                double d18 = temperatures[(i * 4 + i3) * 16 + (j * 4 + i4)];
                                int n8 = 0;
                                if (k * 8 + i2 < n2) {
                                    n8 = d18 < 0.5 && k * 8 + i2 >= n2 - 1 ? Block.ICE.id : Block.WATER.id;
                                }
                                if (d16 > 0.0) {
                                    n8 = Block.STONE.id;
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

    public void buildSurfaces(int chunkX, int chunkZ, byte[] blocks, Biome[] biomes) {
        int n = 64;
        double d = 0.03125;
        this.sandBuffer = this.perlinNoise2.create(this.sandBuffer, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, d, d, 1.0);
        this.gravelBuffer = this.perlinNoise2.create(this.gravelBuffer, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, d, 1.0, d);
        this.depthBuffer = this.perlinNoise3.create(this.depthBuffer, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, d * 2.0, d * 2.0, d * 2.0);
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                Biome biome = biomes[i + j * 16];
                boolean bl = this.sandBuffer[i + j * 16] + this.random.nextDouble() * 0.2 > 0.0;
                boolean bl2 = this.gravelBuffer[i + j * 16] + this.random.nextDouble() * 0.2 > 3.0;
                int n2 = (int)(this.depthBuffer[i + j * 16] / 3.0 + 3.0 + this.random.nextDouble() * 0.25);
                int n3 = -1;
                byte by = biome.topBlockId;
                byte by2 = biome.soilBlockId;
                for (int k = 127; k >= 0; --k) {
                    int n4 = (j * 16 + i) * 128 + k;
                    if (k <= 0 + this.random.nextInt(5)) {
                        blocks[n4] = (byte)Block.BEDROCK.id;
                        continue;
                    }
                    byte by3 = blocks[n4];
                    if (by3 == 0) {
                        n3 = -1;
                        continue;
                    }
                    if (by3 != Block.STONE.id) continue;
                    if (n3 == -1) {
                        if (n2 <= 0) {
                            by = 0;
                            by2 = (byte)Block.STONE.id;
                        } else if (k >= n - 4 && k <= n + 1) {
                            by = biome.topBlockId;
                            by2 = biome.soilBlockId;
                            if (bl2) {
                                by = 0;
                            }
                            if (bl2) {
                                by2 = (byte)Block.GRAVEL.id;
                            }
                            if (bl) {
                                by = (byte)Block.SAND.id;
                            }
                            if (bl) {
                                by2 = (byte)Block.SAND.id;
                            }
                        }
                        if (k < n && by == 0) {
                            by = (byte)Block.WATER.id;
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
                    blocks[n4] = by2;
                    if (--n3 != 0 || by2 != Block.SAND.id) continue;
                    n3 = this.random.nextInt(4);
                    by2 = (byte)Block.SANDSTONE.id;
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
        Chunk chunk = new Chunk(this.world, byArray, chunkX, chunkZ);
        this.biomes = this.world.getBiomeSource().getBiomesInArea(this.biomes, chunkX * 16, chunkZ * 16, 16, 16);
        double[] dArray = this.world.getBiomeSource().temperatureMap;
        this.buildTerrain(chunkX, chunkZ, byArray, this.biomes, dArray);
        this.buildSurfaces(chunkX, chunkZ, byArray, this.biomes);
        this.cave.place(this, this.world, chunkX, chunkZ, byArray);
        chunk.populateHeightMap();
        return chunk;
    }

    private double[] generateHeightMap(double[] heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
        if (heightMap == null) {
            heightMap = new double[sizeX * sizeY * sizeZ];
        }
        double d = 684.412;
        double d2 = 684.412;
        double[] dArray = this.world.getBiomeSource().temperatureMap;
        double[] dArray2 = this.world.getBiomeSource().downfallMap;
        this.scaleNoiseBuffer = this.floatingIslandScale.create(this.scaleNoiseBuffer, x, z, sizeX, sizeZ, 1.121, 1.121, 0.5);
        this.depthNoiseBuffer = this.floatingIslandNoise.create(this.depthNoiseBuffer, x, z, sizeX, sizeZ, 200.0, 200.0, 0.5);
        this.perlinNoiseBuffer = this.perlinNoise1.create(this.perlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d / 80.0, d2 / 160.0, d / 80.0);
        this.minLimitPerlinNoiseBuffer = this.minLimitPerlinNoise.create(this.minLimitPerlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d, d2, d);
        this.maxLimitPerlinNoiseBuffer = this.maxLimitPerlinNoise.create(this.maxLimitPerlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, d, d2, d);
        int n = 0;
        int n2 = 0;
        int n3 = 16 / sizeX;
        for (int i = 0; i < sizeX; ++i) {
            int n4 = i * n3 + n3 / 2;
            for (int j = 0; j < sizeZ; ++j) {
                double d3;
                int n5 = j * n3 + n3 / 2;
                double d4 = dArray[n4 * 16 + n5];
                double d5 = dArray2[n4 * 16 + n5] * d4;
                double d6 = 1.0 - d5;
                d6 *= d6;
                d6 *= d6;
                d6 = 1.0 - d6;
                double d7 = (this.scaleNoiseBuffer[n2] + 256.0) / 512.0;
                if ((d7 *= d6) > 1.0) {
                    d7 = 1.0;
                }
                if ((d3 = this.depthNoiseBuffer[n2] / 8000.0) < 0.0) {
                    d3 = -d3 * 0.3;
                }
                if ((d3 = d3 * 3.0 - 2.0) < 0.0) {
                    if ((d3 /= 2.0) < -1.0) {
                        d3 = -1.0;
                    }
                    d3 /= 1.4;
                    d3 /= 2.0;
                    d7 = 0.0;
                } else {
                    if (d3 > 1.0) {
                        d3 = 1.0;
                    }
                    d3 /= 8.0;
                }
                if (d7 < 0.0) {
                    d7 = 0.0;
                }
                d7 += 0.5;
                d3 = d3 * (double)sizeY / 16.0;
                double d8 = (double)sizeY / 2.0 + d3 * 4.0;
                ++n2;
                for (int k = 0; k < sizeY; ++k) {
                    double d9 = 0.0;
                    double d10 = ((double)k - d8) * 12.0 / d7;
                    if (d10 < 0.0) {
                        d10 *= 4.0;
                    }
                    double d11 = this.minLimitPerlinNoiseBuffer[n] / 512.0;
                    double d12 = this.maxLimitPerlinNoiseBuffer[n] / 512.0;
                    double d13 = (this.perlinNoiseBuffer[n] / 10.0 + 1.0) / 2.0;
                    d9 = d13 < 0.0 ? d11 : (d13 > 1.0 ? d12 : d11 + (d12 - d11) * d13);
                    d9 -= d10;
                    if (k > sizeY - 4) {
                        double d14 = (float)(k - (sizeY - 4)) / 3.0f;
                        d9 = d9 * (1.0 - d14) + -10.0 * d14;
                    }
                    heightMap[n] = d9;
                    ++n;
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
        int n6;
        int n7;
        int n8;
        int n9;
        SandBlock.fallInstantly = true;
        int n10 = x * 16;
        int n11 = z * 16;
        Biome biome = this.world.getBiomeSource().getBiome(n10 + 16, n11 + 16);
        this.random.setSeed(this.world.getSeed());
        long l = this.random.nextLong() / 2L * 2L + 1L;
        long l2 = this.random.nextLong() / 2L * 2L + 1L;
        this.random.setSeed((long)x * l + (long)z * l2 ^ this.world.getSeed());
        double d = 0.25;
        if (this.random.nextInt(4) == 0) {
            n9 = n10 + this.random.nextInt(16) + 8;
            n8 = this.random.nextInt(128);
            n7 = n11 + this.random.nextInt(16) + 8;
            new LakeFeature(Block.WATER.id).generate(this.world, this.random, n9, n8, n7);
        }
        if (this.random.nextInt(8) == 0) {
            n9 = n10 + this.random.nextInt(16) + 8;
            n8 = this.random.nextInt(this.random.nextInt(120) + 8);
            n7 = n11 + this.random.nextInt(16) + 8;
            if (n8 < 64 || this.random.nextInt(10) == 0) {
                new LakeFeature(Block.LAVA.id).generate(this.world, this.random, n9, n8, n7);
            }
        }
        for (n9 = 0; n9 < 8; ++n9) {
            n8 = n10 + this.random.nextInt(16) + 8;
            n7 = this.random.nextInt(128);
            n6 = n11 + this.random.nextInt(16) + 8;
            new DungeonFeature().generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 10; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(128);
            n6 = n11 + this.random.nextInt(16);
            new ClayOreFeature(32).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 20; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(128);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.DIRT.id, 32).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 10; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(128);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.GRAVEL.id, 32).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 20; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(128);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.COAL_ORE.id, 16).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 20; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(64);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.IRON_ORE.id, 8).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 2; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(32);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.GOLD_ORE.id, 8).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 8; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(16);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.REDSTONE_ORE.id, 7).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 1; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(16);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.DIAMOND_ORE.id, 7).generate(this.world, this.random, n8, n7, n6);
        }
        for (n9 = 0; n9 < 1; ++n9) {
            n8 = n10 + this.random.nextInt(16);
            n7 = this.random.nextInt(16) + this.random.nextInt(16);
            n6 = n11 + this.random.nextInt(16);
            new OreFeature(Block.LAPIS_ORE.id, 6).generate(this.world, this.random, n8, n7, n6);
        }
        d = 0.5;
        n9 = (int)((this.forestNoise.sample((double)n10 * d, (double)n11 * d) / 8.0 + this.random.nextDouble() * 4.0 + 4.0) / 3.0);
        n8 = 0;
        if (this.random.nextInt(10) == 0) {
            ++n8;
        }
        if (biome == Biome.FOREST) {
            n8 += n9 + 5;
        }
        if (biome == Biome.RAINFOREST) {
            n8 += n9 + 5;
        }
        if (biome == Biome.SEASONAL_FOREST) {
            n8 += n9 + 2;
        }
        if (biome == Biome.TAIGA) {
            n8 += n9 + 5;
        }
        if (biome == Biome.DESERT) {
            n8 -= 20;
        }
        if (biome == Biome.TUNDRA) {
            n8 -= 20;
        }
        if (biome == Biome.PLAINS) {
            n8 -= 20;
        }
        for (n7 = 0; n7 < n8; ++n7) {
            n6 = n10 + this.random.nextInt(16) + 8;
            n5 = n11 + this.random.nextInt(16) + 8;
            Feature feature = biome.getRandomTreeFeature(this.random);
            feature.prepare(1.0, 1.0, 1.0);
            feature.generate(this.world, this.random, n6, this.world.getTopY(n6, n5), n5);
        }
        n7 = 0;
        if (biome == Biome.FOREST) {
            n7 = 2;
        }
        if (biome == Biome.SEASONAL_FOREST) {
            n7 = 4;
        }
        if (biome == Biome.TAIGA) {
            n7 = 2;
        }
        if (biome == Biome.PLAINS) {
            n7 = 3;
        }
        for (n6 = 0; n6 < n7; ++n6) {
            n5 = n10 + this.random.nextInt(16) + 8;
            int n12 = this.random.nextInt(128);
            n4 = n11 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.DANDELION.id).generate(this.world, this.random, n5, n12, n4);
        }
        n6 = 0;
        if (biome == Biome.FOREST) {
            n6 = 2;
        }
        if (biome == Biome.RAINFOREST) {
            n6 = 10;
        }
        if (biome == Biome.SEASONAL_FOREST) {
            n6 = 2;
        }
        if (biome == Biome.TAIGA) {
            n6 = 1;
        }
        if (biome == Biome.PLAINS) {
            n6 = 10;
        }
        for (n5 = 0; n5 < n6; ++n5) {
            int n13 = 1;
            if (biome == Biome.RAINFOREST && this.random.nextInt(3) != 0) {
                n13 = 2;
            }
            n4 = n10 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n11 + this.random.nextInt(16) + 8;
            new GrassPatchFeature(Block.GRASS.id, n13).generate(this.world, this.random, n4, n3, n2);
        }
        n6 = 0;
        if (biome == Biome.DESERT) {
            n6 = 2;
        }
        for (n5 = 0; n5 < n6; ++n5) {
            int n14 = n10 + this.random.nextInt(16) + 8;
            n4 = this.random.nextInt(128);
            n3 = n11 + this.random.nextInt(16) + 8;
            new DeadBushPatchFeature(Block.DEAD_BUSH.id).generate(this.world, this.random, n14, n4, n3);
        }
        if (this.random.nextInt(2) == 0) {
            n5 = n10 + this.random.nextInt(16) + 8;
            int n15 = this.random.nextInt(128);
            n4 = n11 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.ROSE.id).generate(this.world, this.random, n5, n15, n4);
        }
        if (this.random.nextInt(4) == 0) {
            n5 = n10 + this.random.nextInt(16) + 8;
            int n16 = this.random.nextInt(128);
            n4 = n11 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.BROWN_MUSHROOM.id).generate(this.world, this.random, n5, n16, n4);
        }
        if (this.random.nextInt(8) == 0) {
            n5 = n10 + this.random.nextInt(16) + 8;
            int n17 = this.random.nextInt(128);
            n4 = n11 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.RED_MUSHROOM.id).generate(this.world, this.random, n5, n17, n4);
        }
        for (n5 = 0; n5 < 10; ++n5) {
            int n18 = n10 + this.random.nextInt(16) + 8;
            n4 = this.random.nextInt(128);
            n3 = n11 + this.random.nextInt(16) + 8;
            new SugarCanePatchFeature().generate(this.world, this.random, n18, n4, n3);
        }
        if (this.random.nextInt(32) == 0) {
            n5 = n10 + this.random.nextInt(16) + 8;
            int n19 = this.random.nextInt(128);
            n4 = n11 + this.random.nextInt(16) + 8;
            new PumpkinPatchFeature().generate(this.world, this.random, n5, n19, n4);
        }
        n5 = 0;
        if (biome == Biome.DESERT) {
            n5 += 10;
        }
        for (n = 0; n < n5; ++n) {
            n4 = n10 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n11 + this.random.nextInt(16) + 8;
            new CactusPatchFeature().generate(this.world, this.random, n4, n3, n2);
        }
        for (n = 0; n < 50; ++n) {
            n4 = n10 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(this.random.nextInt(120) + 8);
            n2 = n11 + this.random.nextInt(16) + 8;
            new SpringFeature(Block.FLOWING_WATER.id).generate(this.world, this.random, n4, n3, n2);
        }
        for (n = 0; n < 20; ++n) {
            n4 = n10 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(this.random.nextInt(this.random.nextInt(112) + 8) + 8);
            n2 = n11 + this.random.nextInt(16) + 8;
            new SpringFeature(Block.FLOWING_LAVA.id).generate(this.world, this.random, n4, n3, n2);
        }
        this.temperatures = this.world.getBiomeSource().create(this.temperatures, n10 + 8, n11 + 8, 16, 16);
        for (n = n10 + 8; n < n10 + 8 + 16; ++n) {
            for (n4 = n11 + 8; n4 < n11 + 8 + 16; ++n4) {
                n3 = n - (n10 + 8);
                n2 = n4 - (n11 + 8);
                int n20 = this.world.getTopSolidBlockY(n, n4);
                double d2 = this.temperatures[n3 * 16 + n2] - (double)(n20 - 64) / 64.0 * 0.3;
                if (!(d2 < 0.5) || n20 <= 0 || n20 >= 128 || !this.world.isAir(n, n20, n4) || !this.world.getMaterial(n, n20 - 1, n4).blocksMovement() || this.world.getMaterial(n, n20 - 1, n4) == Material.ICE) continue;
                this.world.setBlock(n, n20, n4, Block.SNOW.id);
            }
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
        return "RandomLevelSource";
    }
}

