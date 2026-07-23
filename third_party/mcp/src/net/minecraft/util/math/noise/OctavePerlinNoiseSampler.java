/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.math.noise;

import java.util.Random;
import net.minecraft.util.math.noise.NoiseSampler;
import net.minecraft.util.math.noise.PerlinNoiseSampler;

public class OctavePerlinNoiseSampler
extends NoiseSampler {
    private PerlinNoiseSampler[] octaveSamplers;
    private int octaves;

    public OctavePerlinNoiseSampler(Random random, int i) {
        this.octaves = i;
        this.octaveSamplers = new PerlinNoiseSampler[i];
        for (int j = 0; j < i; ++j) {
            this.octaveSamplers[j] = new PerlinNoiseSampler(random);
        }
    }

    public double sample(double x, double y) {
        double d = 0.0;
        double d2 = 1.0;
        for (int i = 0; i < this.octaves; ++i) {
            d += this.octaveSamplers[i].sample(x * d2, y * d2) / d2;
            d2 /= 2.0;
        }
        return d;
    }

    public double[] create(double[] map, double x, double y, double z, int width, int height, int depth, double g, double h, double l) {
        if (map == null) {
            map = new double[width * height * depth];
        } else {
            for (int i = 0; i < map.length; ++i) {
                map[i] = 0.0;
            }
        }
        double d = 1.0;
        for (int i = 0; i < this.octaves; ++i) {
            this.octaveSamplers[i].create(map, x, y, z, width, height, depth, g * d, h * d, l * d, d);
            d /= 2.0;
        }
        return map;
    }

    public double[] create(double[] map, int x, int z, int width, int depth, double d, double e, double f) {
        return this.create(map, x, 10.0, z, width, 1, depth, d, 1.0, e);
    }
}

