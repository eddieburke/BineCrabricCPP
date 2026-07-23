/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.math.noise;

import java.util.Random;
import net.minecraft.util.math.noise.NoiseSampler;
import net.minecraft.util.math.noise.SimplexNoiseSampler;

public class OctaveSimplexNoiseSampler
extends NoiseSampler {
    private SimplexNoiseSampler[] octaveSamplers;
    private int octaves;

    public OctaveSimplexNoiseSampler(Random random, int octaves) {
        this.octaves = octaves;
        this.octaveSamplers = new SimplexNoiseSampler[octaves];
        for (int i = 0; i < octaves; ++i) {
            this.octaveSamplers[i] = new SimplexNoiseSampler(random);
        }
    }

    public double[] sample(double[] map, double x, double y, int width, int height, double f, double g, double h) {
        return this.sample(map, x, y, width, height, f, g, h, 0.5);
    }

    public double[] sample(double[] map, double x, double y, int width, int height, double f, double g, double h, double k) {
        f /= 1.5;
        g /= 1.5;
        if (map == null || map.length < width * height) {
            map = new double[width * height];
        } else {
            for (int i = 0; i < map.length; ++i) {
                map[i] = 0.0;
            }
        }
        double d = 1.0;
        double d2 = 1.0;
        for (int i = 0; i < this.octaves; ++i) {
            this.octaveSamplers[i].create(map, x, y, width, height, f * d2, g * d2, 0.55 / d);
            d2 *= h;
            d *= k;
        }
        return map;
    }
}

