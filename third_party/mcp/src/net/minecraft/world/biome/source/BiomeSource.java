/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.biome.source;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.noise.OctaveSimplexNoiseSampler;
import net.minecraft.world.World;
import net.minecraft.world.biome.Biome;

public class BiomeSource {
    private OctaveSimplexNoiseSampler temperatureSampler;
    private OctaveSimplexNoiseSampler downfallSampler;
    private OctaveSimplexNoiseSampler weirdnessSampler;
    public double[] temperatureMap;
    public double[] downfallMap;
    public double[] weirdnessMap;
    public Biome[] biomes;

    protected BiomeSource() {
    }

    public BiomeSource(World world) {
        this.temperatureSampler = new OctaveSimplexNoiseSampler(new Random(world.getSeed() * 9871L), 4);
        this.downfallSampler = new OctaveSimplexNoiseSampler(new Random(world.getSeed() * 39811L), 4);
        this.weirdnessSampler = new OctaveSimplexNoiseSampler(new Random(world.getSeed() * 543321L), 2);
    }

    public Biome getBiome(ChunkPos chunkPos) {
        return this.getBiome(chunkPos.x << 4, chunkPos.z << 4);
    }

    public Biome getBiome(int x, int z) {
        return this.getBiomesInArea(x, z, 1, 1)[0];
    }

    @Environment(value=EnvType.CLIENT)
    public double getTemperature(int x, int z) {
        this.temperatureMap = this.temperatureSampler.sample(this.temperatureMap, x, z, 1, 1, 0.025f, 0.025f, 0.5);
        return this.temperatureMap[0];
    }

    public Biome[] getBiomesInArea(int x, int z, int width, int depth) {
        this.biomes = this.getBiomesInArea(this.biomes, x, z, width, depth);
        return this.biomes;
    }

    public double[] create(double[] map, int x, int z, int width, int depth) {
        if (map == null || map.length < width * depth) {
            map = new double[width * depth];
        }
        map = this.temperatureSampler.sample(map, x, z, width, depth, 0.025f, 0.025f, 0.25);
        this.weirdnessMap = this.weirdnessSampler.sample(this.weirdnessMap, x, z, width, depth, 0.25, 0.25, 0.5882352941176471);
        int n = 0;
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < depth; ++j) {
                double d = this.weirdnessMap[n] * 1.1 + 0.5;
                double d2 = 0.01;
                double d3 = 1.0 - d2;
                double d4 = (map[n] * 0.15 + 0.7) * d3 + d * d2;
                if ((d4 = 1.0 - (1.0 - d4) * (1.0 - d4)) < 0.0) {
                    d4 = 0.0;
                }
                if (d4 > 1.0) {
                    d4 = 1.0;
                }
                map[n] = d4;
                ++n;
            }
        }
        return map;
    }

    public Biome[] getBiomesInArea(Biome[] biomes, int x, int z, int width, int depth) {
        if (biomes == null || biomes.length < width * depth) {
            biomes = new Biome[width * depth];
        }
        this.temperatureMap = this.temperatureSampler.sample(this.temperatureMap, x, z, width, width, 0.025f, 0.025f, 0.25);
        this.downfallMap = this.downfallSampler.sample(this.downfallMap, x, z, width, width, 0.05f, 0.05f, 0.3333333333333333);
        this.weirdnessMap = this.weirdnessSampler.sample(this.weirdnessMap, x, z, width, width, 0.25, 0.25, 0.5882352941176471);
        int n = 0;
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < depth; ++j) {
                double d = this.weirdnessMap[n] * 1.1 + 0.5;
                double d2 = 0.01;
                double d3 = 1.0 - d2;
                double d4 = (this.temperatureMap[n] * 0.15 + 0.7) * d3 + d * d2;
                d2 = 0.002;
                d3 = 1.0 - d2;
                double d5 = (this.downfallMap[n] * 0.15 + 0.5) * d3 + d * d2;
                if ((d4 = 1.0 - (1.0 - d4) * (1.0 - d4)) < 0.0) {
                    d4 = 0.0;
                }
                if (d5 < 0.0) {
                    d5 = 0.0;
                }
                if (d4 > 1.0) {
                    d4 = 1.0;
                }
                if (d5 > 1.0) {
                    d5 = 1.0;
                }
                this.temperatureMap[n] = d4;
                this.downfallMap[n] = d5;
                biomes[n++] = Biome.getBiome(d4, d5);
            }
        }
        return biomes;
    }
}

