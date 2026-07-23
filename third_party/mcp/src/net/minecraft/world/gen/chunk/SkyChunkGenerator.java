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
import net.minecraft.world.gen.feature.DungeonFeature;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.LakeFeature;
import net.minecraft.world.gen.feature.OreFeature;
import net.minecraft.world.gen.feature.PlantPatchFeature;
import net.minecraft.world.gen.feature.PumpkinPatchFeature;
import net.minecraft.world.gen.feature.SpringFeature;
import net.minecraft.world.gen.feature.SugarCanePatchFeature;

public class SkyChunkGenerator
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

    public SkyChunkGenerator(World world, long seed) {
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
        int n = 2;
        int n2 = n + 1;
        int n3 = 33;
        int n4 = n + 1;
        this.heightMap = this.generateHeightMap(this.heightMap, chunkX * n, 0, chunkZ * n, n2, n3, n4);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < 32; ++k) {
                    double d = 0.25;
                    double d2 = this.heightMap[((i + 0) * n4 + (j + 0)) * n3 + (k + 0)];
                    double d3 = this.heightMap[((i + 0) * n4 + (j + 1)) * n3 + (k + 0)];
                    double d4 = this.heightMap[((i + 1) * n4 + (j + 0)) * n3 + (k + 0)];
                    double d5 = this.heightMap[((i + 1) * n4 + (j + 1)) * n3 + (k + 0)];
                    double d6 = (this.heightMap[((i + 0) * n4 + (j + 0)) * n3 + (k + 1)] - d2) * d;
                    double d7 = (this.heightMap[((i + 0) * n4 + (j + 1)) * n3 + (k + 1)] - d3) * d;
                    double d8 = (this.heightMap[((i + 1) * n4 + (j + 0)) * n3 + (k + 1)] - d4) * d;
                    double d9 = (this.heightMap[((i + 1) * n4 + (j + 1)) * n3 + (k + 1)] - d5) * d;
                    for (int i2 = 0; i2 < 4; ++i2) {
                        double d10 = 0.125;
                        double d11 = d2;
                        double d12 = d3;
                        double d13 = (d4 - d2) * d10;
                        double d14 = (d5 - d3) * d10;
                        for (int i3 = 0; i3 < 8; ++i3) {
                            int n5 = i3 + i * 8 << 11 | 0 + j * 8 << 7 | k * 4 + i2;
                            int n6 = 128;
                            double d15 = 0.125;
                            double d16 = d11;
                            double d17 = (d12 - d11) * d15;
                            for (int i4 = 0; i4 < 8; ++i4) {
                                int n7 = 0;
                                if (d16 > 0.0) {
                                    n7 = Block.STONE.id;
                                }
                                blocks[n5] = (byte)n7;
                                n5 += n6;
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
        double d = 0.03125;
        this.sandBuffer = this.perlinNoise2.create(this.sandBuffer, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, d, d, 1.0);
        this.gravelBuffer = this.perlinNoise2.create(this.gravelBuffer, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, d, 1.0, d);
        this.depthBuffer = this.perlinNoise3.create(this.depthBuffer, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, d * 2.0, d * 2.0, d * 2.0);
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                Biome biome = biomes[i + j * 16];
                int n = (int)(this.depthBuffer[i + j * 16] / 3.0 + 3.0 + this.random.nextDouble() * 0.25);
                int n2 = -1;
                byte by = biome.topBlockId;
                byte by2 = biome.soilBlockId;
                for (int k = 127; k >= 0; --k) {
                    int n3 = (j * 16 + i) * 128 + k;
                    byte by3 = blocks[n3];
                    if (by3 == 0) {
                        n2 = -1;
                        continue;
                    }
                    if (by3 != Block.STONE.id) continue;
                    if (n2 == -1) {
                        if (n <= 0) {
                            by = 0;
                            by2 = (byte)Block.STONE.id;
                        }
                        n2 = n;
                        if (k >= 0) {
                            blocks[n3] = by;
                            continue;
                        }
                        blocks[n3] = by2;
                        continue;
                    }
                    if (n2 <= 0) continue;
                    blocks[n3] = by2;
                    if (--n2 != 0 || by2 != Block.SAND.id) continue;
                    n2 = this.random.nextInt(4);
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
        this.perlinNoiseBuffer = this.perlinNoise1.create(this.perlinNoiseBuffer, x, y, z, sizeX, sizeY, sizeZ, (d *= 2.0) / 80.0, d2 / 160.0, d / 80.0);
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
                if ((d3 = d3 * 3.0 - 2.0) > 1.0) {
                    d3 = 1.0;
                }
                d3 /= 8.0;
                d3 = 0.0;
                if (d7 < 0.0) {
                    d7 = 0.0;
                }
                d7 += 0.5;
                d3 = d3 * (double)sizeY / 16.0;
                ++n2;
                double d8 = (double)sizeY / 2.0;
                for (int k = 0; k < sizeY; ++k) {
                    double d9;
                    double d10 = 0.0;
                    double d11 = ((double)k - d8) * 8.0 / d7;
                    if (d11 < 0.0) {
                        d11 *= -1.0;
                    }
                    double d12 = this.minLimitPerlinNoiseBuffer[n] / 512.0;
                    double d13 = this.maxLimitPerlinNoiseBuffer[n] / 512.0;
                    double d14 = (this.perlinNoiseBuffer[n] / 10.0 + 1.0) / 2.0;
                    d10 = d14 < 0.0 ? d12 : (d14 > 1.0 ? d13 : d12 + (d13 - d12) * d14);
                    d10 -= 8.0;
                    int n6 = 32;
                    if (k > sizeY - n6) {
                        d9 = (float)(k - (sizeY - n6)) / ((float)n6 - 1.0f);
                        d10 = d10 * (1.0 - d9) + -30.0 * d9;
                    }
                    if (k < (n6 = 8)) {
                        d9 = (float)(n6 - k) / ((float)n6 - 1.0f);
                        d10 = d10 * (1.0 - d9) + -30.0 * d9;
                    }
                    heightMap[n] = d10;
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
        SandBlock.fallInstantly = true;
        int n7 = x * 16;
        int n8 = z * 16;
        Biome biome = this.world.getBiomeSource().getBiome(n7 + 16, n8 + 16);
        this.random.setSeed(this.world.getSeed());
        long l = this.random.nextLong() / 2L * 2L + 1L;
        long l2 = this.random.nextLong() / 2L * 2L + 1L;
        this.random.setSeed((long)x * l + (long)z * l2 ^ this.world.getSeed());
        double d = 0.25;
        if (this.random.nextInt(4) == 0) {
            n6 = n7 + this.random.nextInt(16) + 8;
            n5 = this.random.nextInt(128);
            n4 = n8 + this.random.nextInt(16) + 8;
            new LakeFeature(Block.WATER.id).generate(this.world, this.random, n6, n5, n4);
        }
        if (this.random.nextInt(8) == 0) {
            n6 = n7 + this.random.nextInt(16) + 8;
            n5 = this.random.nextInt(this.random.nextInt(120) + 8);
            n4 = n8 + this.random.nextInt(16) + 8;
            if (n5 < 64 || this.random.nextInt(10) == 0) {
                new LakeFeature(Block.LAVA.id).generate(this.world, this.random, n6, n5, n4);
            }
        }
        for (n6 = 0; n6 < 8; ++n6) {
            n5 = n7 + this.random.nextInt(16) + 8;
            n4 = this.random.nextInt(128);
            n3 = n8 + this.random.nextInt(16) + 8;
            new DungeonFeature().generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 10; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(128);
            n3 = n8 + this.random.nextInt(16);
            new ClayOreFeature(32).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 20; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(128);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.DIRT.id, 32).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 10; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(128);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.GRAVEL.id, 32).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 20; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(128);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.COAL_ORE.id, 16).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 20; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(64);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.IRON_ORE.id, 8).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 2; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(32);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.GOLD_ORE.id, 8).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 8; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(16);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.REDSTONE_ORE.id, 7).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 1; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(16);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.DIAMOND_ORE.id, 7).generate(this.world, this.random, n5, n4, n3);
        }
        for (n6 = 0; n6 < 1; ++n6) {
            n5 = n7 + this.random.nextInt(16);
            n4 = this.random.nextInt(16) + this.random.nextInt(16);
            n3 = n8 + this.random.nextInt(16);
            new OreFeature(Block.LAPIS_ORE.id, 6).generate(this.world, this.random, n5, n4, n3);
        }
        d = 0.5;
        n6 = (int)((this.forestNoise.sample((double)n7 * d, (double)n8 * d) / 8.0 + this.random.nextDouble() * 4.0 + 4.0) / 3.0);
        n5 = 0;
        if (this.random.nextInt(10) == 0) {
            ++n5;
        }
        if (biome == Biome.FOREST) {
            n5 += n6 + 5;
        }
        if (biome == Biome.RAINFOREST) {
            n5 += n6 + 5;
        }
        if (biome == Biome.SEASONAL_FOREST) {
            n5 += n6 + 2;
        }
        if (biome == Biome.TAIGA) {
            n5 += n6 + 5;
        }
        if (biome == Biome.DESERT) {
            n5 -= 20;
        }
        if (biome == Biome.TUNDRA) {
            n5 -= 20;
        }
        if (biome == Biome.PLAINS) {
            n5 -= 20;
        }
        for (n4 = 0; n4 < n5; ++n4) {
            n3 = n7 + this.random.nextInt(16) + 8;
            n2 = n8 + this.random.nextInt(16) + 8;
            Feature feature = biome.getRandomTreeFeature(this.random);
            feature.prepare(1.0, 1.0, 1.0);
            feature.generate(this.world, this.random, n3, this.world.getTopY(n3, n2), n2);
        }
        for (n4 = 0; n4 < 2; ++n4) {
            n3 = n7 + this.random.nextInt(16) + 8;
            n2 = this.random.nextInt(128);
            int n9 = n8 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.DANDELION.id).generate(this.world, this.random, n3, n2, n9);
        }
        if (this.random.nextInt(2) == 0) {
            n4 = n7 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n8 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.ROSE.id).generate(this.world, this.random, n4, n3, n2);
        }
        if (this.random.nextInt(4) == 0) {
            n4 = n7 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n8 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.BROWN_MUSHROOM.id).generate(this.world, this.random, n4, n3, n2);
        }
        if (this.random.nextInt(8) == 0) {
            n4 = n7 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n8 + this.random.nextInt(16) + 8;
            new PlantPatchFeature(Block.RED_MUSHROOM.id).generate(this.world, this.random, n4, n3, n2);
        }
        for (n4 = 0; n4 < 10; ++n4) {
            n3 = n7 + this.random.nextInt(16) + 8;
            n2 = this.random.nextInt(128);
            int n10 = n8 + this.random.nextInt(16) + 8;
            new SugarCanePatchFeature().generate(this.world, this.random, n3, n2, n10);
        }
        if (this.random.nextInt(32) == 0) {
            n4 = n7 + this.random.nextInt(16) + 8;
            n3 = this.random.nextInt(128);
            n2 = n8 + this.random.nextInt(16) + 8;
            new PumpkinPatchFeature().generate(this.world, this.random, n4, n3, n2);
        }
        n4 = 0;
        if (biome == Biome.DESERT) {
            n4 += 10;
        }
        for (n3 = 0; n3 < n4; ++n3) {
            n2 = n7 + this.random.nextInt(16) + 8;
            int n11 = this.random.nextInt(128);
            n = n8 + this.random.nextInt(16) + 8;
            new CactusPatchFeature().generate(this.world, this.random, n2, n11, n);
        }
        for (n3 = 0; n3 < 50; ++n3) {
            n2 = n7 + this.random.nextInt(16) + 8;
            int n12 = this.random.nextInt(this.random.nextInt(120) + 8);
            n = n8 + this.random.nextInt(16) + 8;
            new SpringFeature(Block.FLOWING_WATER.id).generate(this.world, this.random, n2, n12, n);
        }
        for (n3 = 0; n3 < 20; ++n3) {
            n2 = n7 + this.random.nextInt(16) + 8;
            int n13 = this.random.nextInt(this.random.nextInt(this.random.nextInt(112) + 8) + 8);
            n = n8 + this.random.nextInt(16) + 8;
            new SpringFeature(Block.FLOWING_LAVA.id).generate(this.world, this.random, n2, n13, n);
        }
        this.temperatures = this.world.getBiomeSource().create(this.temperatures, n7 + 8, n8 + 8, 16, 16);
        for (n3 = n7 + 8; n3 < n7 + 8 + 16; ++n3) {
            for (n2 = n8 + 8; n2 < n8 + 8 + 16; ++n2) {
                int n14 = n3 - (n7 + 8);
                n = n2 - (n8 + 8);
                int n15 = this.world.getTopSolidBlockY(n3, n2);
                double d2 = this.temperatures[n14 * 16 + n] - (double)(n15 - 64) / 64.0 * 0.3;
                if (!(d2 < 0.5) || n15 <= 0 || n15 >= 128 || !this.world.isAir(n3, n15, n2) || !this.world.getMaterial(n3, n15 - 1, n2).blocksMovement() || this.world.getMaterial(n3, n15 - 1, n2) == Material.ICE) continue;
                this.world.setBlock(n3, n15, n2, Block.SNOW.id);
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

