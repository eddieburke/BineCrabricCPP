/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.math.noise;

import java.util.Random;
import net.minecraft.util.math.noise.NoiseSampler;

public class PerlinNoiseSampler
extends NoiseSampler {
    private int[] perm = new int[512];
    public double offsetX;
    public double offsetY;
    public double offsetZ;

    public PerlinNoiseSampler() {
        this(new Random());
    }

    public PerlinNoiseSampler(Random random) {
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

    public double sample(double x, double y, double z) {
        double d = x + this.offsetX;
        double d2 = y + this.offsetY;
        double d3 = z + this.offsetZ;
        int n = (int)d;
        int n2 = (int)d2;
        int n3 = (int)d3;
        if (d < (double)n) {
            --n;
        }
        if (d2 < (double)n2) {
            --n2;
        }
        if (d3 < (double)n3) {
            --n3;
        }
        int n4 = n & 0xFF;
        int n5 = n2 & 0xFF;
        int n6 = n3 & 0xFF;
        double d4 = (d -= (double)n) * d * d * (d * (d * 6.0 - 15.0) + 10.0);
        double d5 = (d2 -= (double)n2) * d2 * d2 * (d2 * (d2 * 6.0 - 15.0) + 10.0);
        double d6 = (d3 -= (double)n3) * d3 * d3 * (d3 * (d3 * 6.0 - 15.0) + 10.0);
        int n7 = this.perm[n4] + n5;
        int n8 = this.perm[n7] + n6;
        int n9 = this.perm[n7 + 1] + n6;
        int n10 = this.perm[n4 + 1] + n5;
        int n11 = this.perm[n10] + n6;
        int n12 = this.perm[n10 + 1] + n6;
        return this.lerp(d6, this.lerp(d5, this.lerp(d4, this.gradCoord(this.perm[n8], d, d2, d3), this.gradCoord(this.perm[n11], d - 1.0, d2, d3)), this.lerp(d4, this.gradCoord(this.perm[n9], d, d2 - 1.0, d3), this.gradCoord(this.perm[n12], d - 1.0, d2 - 1.0, d3))), this.lerp(d5, this.lerp(d4, this.gradCoord(this.perm[n8 + 1], d, d2, d3 - 1.0), this.gradCoord(this.perm[n11 + 1], d - 1.0, d2, d3 - 1.0)), this.lerp(d4, this.gradCoord(this.perm[n9 + 1], d, d2 - 1.0, d3 - 1.0), this.gradCoord(this.perm[n12 + 1], d - 1.0, d2 - 1.0, d3 - 1.0))));
    }

    public final double lerp(double x, double y, double t) {
        return y + x * (t - y);
    }

    public final double gradCoord(int i, double x, double y) {
        int n = i & 0xF;
        double d = (double)(1 - ((n & 8) >> 3)) * x;
        double d2 = n < 4 ? 0.0 : (n == 12 || n == 14 ? x : y);
        return ((n & 1) == 0 ? d : -d) + ((n & 2) == 0 ? d2 : -d2);
    }

    public final double gradCoord(int i, double x, double y, double z) {
        double d;
        int n = i & 0xF;
        double d2 = d = n < 8 ? x : y;
        double d3 = n < 4 ? y : (n == 12 || n == 14 ? x : z);
        return ((n & 1) == 0 ? d : -d) + ((n & 2) == 0 ? d3 : -d3);
    }

    public double sample(double x, double y) {
        return this.sample(x, y, 0.0);
    }

    public void create(double[] map, double x, double y, double z, int width, int height, int depth, double g, double h, double l, double m) {
        if (height == 1) {
            int n = 0;
            int n2 = 0;
            int n3 = 0;
            int n4 = 0;
            double d = 0.0;
            double d2 = 0.0;
            int n5 = 0;
            double d3 = 1.0 / m;
            for (int i = 0; i < width; ++i) {
                double d4 = (x + (double)i) * g + this.offsetX;
                int n6 = (int)d4;
                if (d4 < (double)n6) {
                    --n6;
                }
                int n7 = n6 & 0xFF;
                double d5 = (d4 -= (double)n6) * d4 * d4 * (d4 * (d4 * 6.0 - 15.0) + 10.0);
                for (int j = 0; j < depth; ++j) {
                    double d6 = (z + (double)j) * l + this.offsetZ;
                    int n8 = (int)d6;
                    if (d6 < (double)n8) {
                        --n8;
                    }
                    int n9 = n8 & 0xFF;
                    double d7 = (d6 -= (double)n8) * d6 * d6 * (d6 * (d6 * 6.0 - 15.0) + 10.0);
                    n = this.perm[n7] + 0;
                    n2 = this.perm[n] + n9;
                    n3 = this.perm[n7 + 1] + 0;
                    n4 = this.perm[n3] + n9;
                    d = this.lerp(d5, this.gradCoord(this.perm[n2], d4, d6), this.gradCoord(this.perm[n4], d4 - 1.0, 0.0, d6));
                    d2 = this.lerp(d5, this.gradCoord(this.perm[n2 + 1], d4, 0.0, d6 - 1.0), this.gradCoord(this.perm[n4 + 1], d4 - 1.0, 0.0, d6 - 1.0));
                    double d8 = this.lerp(d7, d, d2);
                    int n10 = n5++;
                    map[n10] = map[n10] + d8 * d3;
                }
            }
            return;
        }
        int n = 0;
        double d = 1.0 / m;
        int n11 = -1;
        int n12 = 0;
        int n13 = 0;
        int n14 = 0;
        int n15 = 0;
        int n16 = 0;
        int n17 = 0;
        double d9 = 0.0;
        double d10 = 0.0;
        double d11 = 0.0;
        double d12 = 0.0;
        for (int i = 0; i < width; ++i) {
            double d13 = (x + (double)i) * g + this.offsetX;
            int n18 = (int)d13;
            if (d13 < (double)n18) {
                --n18;
            }
            int n19 = n18 & 0xFF;
            double d14 = (d13 -= (double)n18) * d13 * d13 * (d13 * (d13 * 6.0 - 15.0) + 10.0);
            for (int j = 0; j < depth; ++j) {
                double d15 = (z + (double)j) * l + this.offsetZ;
                int n20 = (int)d15;
                if (d15 < (double)n20) {
                    --n20;
                }
                int n21 = n20 & 0xFF;
                double d16 = (d15 -= (double)n20) * d15 * d15 * (d15 * (d15 * 6.0 - 15.0) + 10.0);
                for (int k = 0; k < height; ++k) {
                    double d17 = (y + (double)k) * h + this.offsetY;
                    int n22 = (int)d17;
                    if (d17 < (double)n22) {
                        --n22;
                    }
                    int n23 = n22 & 0xFF;
                    double d18 = (d17 -= (double)n22) * d17 * d17 * (d17 * (d17 * 6.0 - 15.0) + 10.0);
                    if (k == 0 || n23 != n11) {
                        n11 = n23;
                        n12 = this.perm[n19] + n23;
                        n13 = this.perm[n12] + n21;
                        n14 = this.perm[n12 + 1] + n21;
                        n15 = this.perm[n19 + 1] + n23;
                        n16 = this.perm[n15] + n21;
                        n17 = this.perm[n15 + 1] + n21;
                        d9 = this.lerp(d14, this.gradCoord(this.perm[n13], d13, d17, d15), this.gradCoord(this.perm[n16], d13 - 1.0, d17, d15));
                        d10 = this.lerp(d14, this.gradCoord(this.perm[n14], d13, d17 - 1.0, d15), this.gradCoord(this.perm[n17], d13 - 1.0, d17 - 1.0, d15));
                        d11 = this.lerp(d14, this.gradCoord(this.perm[n13 + 1], d13, d17, d15 - 1.0), this.gradCoord(this.perm[n16 + 1], d13 - 1.0, d17, d15 - 1.0));
                        d12 = this.lerp(d14, this.gradCoord(this.perm[n14 + 1], d13, d17 - 1.0, d15 - 1.0), this.gradCoord(this.perm[n17 + 1], d13 - 1.0, d17 - 1.0, d15 - 1.0));
                    }
                    double d19 = this.lerp(d18, d9, d10);
                    double d20 = this.lerp(d18, d11, d12);
                    double d21 = this.lerp(d16, d19, d20);
                    int n24 = n++;
                    map[n24] = map[n24] + d21 * d;
                }
            }
        }
    }
}

