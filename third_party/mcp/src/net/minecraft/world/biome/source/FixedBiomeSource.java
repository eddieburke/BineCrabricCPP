/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.biome.source;

import java.util.Arrays;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.source.BiomeSource;

public class FixedBiomeSource
extends BiomeSource {
    private Biome biome;
    private double temperature;
    private double downfall;

    public FixedBiomeSource(Biome biome, double temperature, double downfall) {
        this.biome = biome;
        this.temperature = temperature;
        this.downfall = downfall;
    }

    public Biome getBiome(ChunkPos chunkPos) {
        return this.biome;
    }

    public Biome getBiome(int x, int z) {
        return this.biome;
    }

    @Environment(value=EnvType.CLIENT)
    public double getTemperature(int x, int z) {
        return this.temperature;
    }

    public Biome[] getBiomesInArea(int x, int z, int width, int depth) {
        this.biomes = this.getBiomesInArea(this.biomes, x, z, width, depth);
        return this.biomes;
    }

    public double[] create(double[] map, int x, int z, int width, int depth) {
        if (map == null || map.length < width * depth) {
            map = new double[width * depth];
        }
        Arrays.fill(map, 0, width * depth, this.temperature);
        return map;
    }

    public Biome[] getBiomesInArea(Biome[] biomes, int x, int z, int width, int depth) {
        if (biomes == null || biomes.length < width * depth) {
            biomes = new Biome[width * depth];
        }
        if (this.temperatureMap == null || this.temperatureMap.length < width * depth) {
            this.temperatureMap = new double[width * depth];
            this.downfallMap = new double[width * depth];
        }
        Arrays.fill(biomes, 0, width * depth, this.biome);
        Arrays.fill(this.downfallMap, 0, width * depth, this.downfall);
        Arrays.fill(this.temperatureMap, 0, width * depth, this.temperature);
        return biomes;
    }
}

