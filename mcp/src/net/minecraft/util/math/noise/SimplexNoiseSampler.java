/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.math.noise;

import java.util.Random;

public class SimplexNoiseSampler {
    private static int[][] grads = new int[][]{{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}};
    private int[] perm = new int[512];
    public double offsetX;
    public double offsetY;
    public double offsetZ;
    private static final double F2 = 0.5 * (Math.sqrt(3.0) - 1.0);
    private static final double G2 = (3.0 - Math.sqrt(3.0)) / 6.0;

    public SimplexNoiseSampler() {
        this(new Random());
    }

    public SimplexNoiseSampler(Random random) {
        int n;
        this.offsetX = random.nextDouble() * 256.0;
        this.offsetY = random.nextDouble() * 256.0;
        this.offsetZ = random.nextDouble() * 256.0;
        for (n = 0; n < 256; ++n) {
            this.perm[n] = n;
        }
        for (n = 0; n < 256; ++n) {
            int n2 = random.nextInt(256 - n) + n;
            int n3 = this.perm[n];
            this.perm[n] = this.perm[n2];
            this.perm[n2] = n3;
            this.perm[n + 256] = this.perm[n];
        }
    }

    private static int fastFloor(double x) {
        return x > 0.0 ? (int)x : (int)x - 1;
    }

    private static double dot(int[] grad, double x, double y) {
        return (double)grad[0] * x + (double)grad[1] * y;
    }

    public void create(double[] map, double x, double z, int width, int depth, double f, double g, double h) {
        int n = 0;
        for (int i = 0; i < width; ++i) {
            double d = (x + (double)i) * f + this.offsetX;
            for (int j = 0; j < depth; ++j) {
                double d2;
                double d3;
                double d4;
                int n2;
                int n3;
                double d5;
                double d6;
                int n4;
                double d7;
                double d8 = (z + (double)j) * g + this.offsetY;
                double d9 = (d + d8) * F2;
                int n5 = SimplexNoiseSampler.fastFloor(d + d9);
                double d10 = (double)n5 - (d7 = (double)(n5 + (n4 = SimplexNoiseSampler.fastFloor(d8 + d9))) * G2);
                double d11 = d - d10;
                if (d11 > (d6 = d8 - (d5 = (double)n4 - d7))) {
                    n3 = 1;
                    n2 = 0;
                } else {
                    n3 = 0;
                    n2 = 1;
                }
                double d12 = d11 - (double)n3 + G2;
                double d13 = d6 - (double)n2 + G2;
                double d14 = d11 - 1.0 + 2.0 * G2;
                double d15 = d6 - 1.0 + 2.0 * G2;
                int n6 = n5 & 0xFF;
                int n7 = n4 & 0xFF;
                int n8 = this.perm[n6 + this.perm[n7]] % 12;
                int n9 = this.perm[n6 + n3 + this.perm[n7 + n2]] % 12;
                int n10 = this.perm[n6 + 1 + this.perm[n7 + 1]] % 12;
                double d16 = 0.5 - d11 * d11 - d6 * d6;
                if (d16 < 0.0) {
                    d4 = 0.0;
                } else {
                    d16 *= d16;
                    d4 = d16 * d16 * SimplexNoiseSampler.dot(grads[n8], d11, d6);
                }
                double d17 = 0.5 - d12 * d12 - d13 * d13;
                if (d17 < 0.0) {
                    d3 = 0.0;
                } else {
                    d17 *= d17;
                    d3 = d17 * d17 * SimplexNoiseSampler.dot(grads[n9], d12, d13);
                }
                double d18 = 0.5 - d14 * d14 - d15 * d15;
                if (d18 < 0.0) {
                    d2 = 0.0;
                } else {
                    d18 *= d18;
                    d2 = d18 * d18 * SimplexNoiseSampler.dot(grads[n10], d14, d15);
                }
                int n11 = n++;
                map[n11] = map[n11] + 70.0 * (d4 + d3 + d2) * h;
            }
        }
    }
}

