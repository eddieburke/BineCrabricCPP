/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class LargeOakTreeFeature
extends Feature {
    static final byte[] MINOR_AXES = new byte[]{2, 0, 0, 1, 2, 1};
    Random random = new Random();
    World world;
    int[] origin = new int[]{0, 0, 0};
    int height = 0;
    int trunkHeight;
    double trunkScale = 0.618;
    double branchDensity = 1.0;
    double branchSlope = 0.381;
    double branchLengthScale = 1.0;
    double foliageDensity = 1.0;
    int trunkWidth = 1;
    int maxTrunkHeight = 12;
    int foliageClusterHeight = 4;
    int[][] branches;

    void makeBranches() {
        int n;
        this.trunkHeight = (int)((double)this.height * this.trunkScale);
        if (this.trunkHeight >= this.height) {
            this.trunkHeight = this.height - 1;
        }
        if ((n = (int)(1.382 + Math.pow(this.foliageDensity * (double)this.height / 13.0, 2.0))) < 1) {
            n = 1;
        }
        int[][] nArray = new int[n * this.height][4];
        int n2 = this.origin[1] + this.height - this.foliageClusterHeight;
        int n3 = 1;
        int n4 = this.origin[1] + this.trunkHeight;
        int n5 = n2 - this.origin[1];
        nArray[0][0] = this.origin[0];
        nArray[0][1] = n2--;
        nArray[0][2] = this.origin[2];
        nArray[0][3] = n4;
        while (n5 >= 0) {
            float f = this.getTreeShape(n5);
            if (f < 0.0f) {
                --n2;
                --n5;
                continue;
            }
            double d = 0.5;
            for (int i = 0; i < n; ++i) {
                int[] nArray2;
                int n6;
                double d2;
                double d3 = this.branchLengthScale * ((double)f * ((double)this.random.nextFloat() + 0.328));
                int n7 = MathHelper.floor(d3 * Math.sin(d2 = (double)this.random.nextFloat() * 2.0 * 3.14159) + (double)this.origin[0] + d);
                int[] nArray3 = new int[]{n7, n2, n6 = MathHelper.floor(d3 * Math.cos(d2) + (double)this.origin[2] + d)};
                if (this.tryBranch(nArray3, nArray2 = new int[]{n7, n2 + this.foliageClusterHeight, n6}) != -1) continue;
                int[] nArray4 = new int[]{this.origin[0], this.origin[1], this.origin[2]};
                double d4 = Math.sqrt(Math.pow(Math.abs(this.origin[0] - nArray3[0]), 2.0) + Math.pow(Math.abs(this.origin[2] - nArray3[2]), 2.0));
                double d5 = d4 * this.branchSlope;
                nArray4[1] = (double)nArray3[1] - d5 > (double)n4 ? n4 : (int)((double)nArray3[1] - d5);
                if (this.tryBranch(nArray4, nArray3) != -1) continue;
                nArray[n3][0] = n7;
                nArray[n3][1] = n2;
                nArray[n3][2] = n6;
                nArray[n3][3] = nArray4[1];
                ++n3;
            }
            --n2;
            --n5;
        }
        this.branches = new int[n3][4];
        System.arraycopy(nArray, 0, this.branches, 0, n3);
    }

    void placeCluster(int x, int y, int z, float shape, byte majorAxis, int clusterBlock) {
        int n = (int)((double)shape + 0.618);
        byte by = MINOR_AXES[majorAxis];
        byte by2 = MINOR_AXES[majorAxis + 3];
        int[] nArray = new int[]{x, y, z};
        int[] nArray2 = new int[]{0, 0, 0};
        int n2 = -n;
        nArray2[majorAxis] = nArray[majorAxis];
        for (int i = -n; i <= n; ++i) {
            nArray2[by] = nArray[by] + i;
            n2 = -n;
            while (n2 <= n) {
                double d = Math.sqrt(Math.pow((double)Math.abs(i) + 0.5, 2.0) + Math.pow((double)Math.abs(n2) + 0.5, 2.0));
                if (d > (double)shape) {
                    ++n2;
                    continue;
                }
                nArray2[by2] = nArray[by2] + n2;
                int n3 = this.world.getBlockId(nArray2[0], nArray2[1], nArray2[2]);
                if (n3 != 0 && n3 != 18) {
                    ++n2;
                    continue;
                }
                this.world.setBlockWithoutNotifyingNeighbors(nArray2[0], nArray2[1], nArray2[2], clusterBlock);
                ++n2;
            }
        }
    }

    float getTreeShape(int height) {
        if ((double)height < (double)this.height * 0.3) {
            return -1.618f;
        }
        float f = (float)this.height / 2.0f;
        float f2 = (float)this.height / 2.0f - (float)height;
        float f3 = f2 == 0.0f ? f : (Math.abs(f2) >= f ? 0.0f : (float)Math.sqrt(Math.pow(Math.abs(f), 2.0) - Math.pow(Math.abs(f2), 2.0)));
        return f3 *= 0.5f;
    }

    float getClusterShape(int layer) {
        if (layer < 0 || layer >= this.foliageClusterHeight) {
            return -1.0f;
        }
        if (layer == 0 || layer == this.foliageClusterHeight - 1) {
            return 2.0f;
        }
        return 3.0f;
    }

    void placeFoliageCluster(int x, int baseY, int z) {
        int n = baseY + this.foliageClusterHeight;
        for (int i = baseY; i < n; ++i) {
            float f = this.getClusterShape(i - baseY);
            this.placeCluster(x, i, z, f, (byte)1, 18);
        }
    }

    void placeBranch(int[] from, int[] to, int log) {
        int[] nArray = new int[]{0, 0, 0};
        int n = 0;
        for (int n2 = 0; n2 < 3; n2 = (int)((byte)(n2 + 1))) {
            nArray[n2] = to[n2] - from[n2];
            if (Math.abs(nArray[n2]) <= Math.abs(nArray[n])) continue;
            n = n2;
        }
        if (nArray[n] == 0) {
            return;
        }
        byte by = MINOR_AXES[n];
        byte by2 = MINOR_AXES[n + 3];
        int n3 = nArray[n] > 0 ? 1 : -1;
        double d = (double)nArray[by] / (double)nArray[n];
        double d2 = (double)nArray[by2] / (double)nArray[n];
        int[] nArray2 = new int[]{0, 0, 0};
        int n4 = nArray[n] + n3;
        for (int i = 0; i != n4; i += n3) {
            nArray2[n] = MathHelper.floor((double)(from[n] + i) + 0.5);
            nArray2[by] = MathHelper.floor((double)from[by] + (double)i * d + 0.5);
            nArray2[by2] = MathHelper.floor((double)from[by2] + (double)i * d2 + 0.5);
            this.world.setBlockWithoutNotifyingNeighbors(nArray2[0], nArray2[1], nArray2[2], log);
        }
    }

    void placeFoliage() {
        int n = this.branches.length;
        for (int i = 0; i < n; ++i) {
            int n2 = this.branches[i][0];
            int n3 = this.branches[i][1];
            int n4 = this.branches[i][2];
            this.placeFoliageCluster(n2, n3, n4);
        }
    }

    boolean shouldPlaceBranch(int height) {
        return !((double)height < (double)this.height * 0.2);
    }

    void PlaceTrunk() {
        int n = this.origin[0];
        int n2 = this.origin[1];
        int n3 = this.origin[1] + this.trunkHeight;
        int n4 = this.origin[2];
        int[] nArray = new int[]{n, n2, n4};
        int[] nArray2 = new int[]{n, n3, n4};
        this.placeBranch(nArray, nArray2, 17);
        if (this.trunkWidth == 2) {
            nArray[0] = nArray[0] + 1;
            nArray2[0] = nArray2[0] + 1;
            this.placeBranch(nArray, nArray2, 17);
            nArray[2] = nArray[2] + 1;
            nArray2[2] = nArray2[2] + 1;
            this.placeBranch(nArray, nArray2, 17);
            nArray[0] = nArray[0] + -1;
            nArray2[0] = nArray2[0] + -1;
            this.placeBranch(nArray, nArray2, 17);
        }
    }

    void placeBranches() {
        int n = this.branches.length;
        int[] nArray = new int[]{this.origin[0], this.origin[1], this.origin[2]};
        for (int i = 0; i < n; ++i) {
            int[] nArray2 = this.branches[i];
            int[] nArray3 = new int[]{nArray2[0], nArray2[1], nArray2[2]};
            nArray[1] = nArray2[3];
            int n2 = nArray[1] - this.origin[1];
            if (!this.shouldPlaceBranch(n2)) continue;
            this.placeBranch(nArray, nArray3, 17);
        }
    }

    int tryBranch(int[] from, int[] to) {
        int n;
        int[] nArray = new int[]{0, 0, 0};
        int n2 = 0;
        for (int n3 = 0; n3 < 3; n3 = (int)((byte)(n3 + 1))) {
            nArray[n3] = to[n3] - from[n3];
            if (Math.abs(nArray[n3]) <= Math.abs(nArray[n2])) continue;
            n2 = n3;
        }
        if (nArray[n2] == 0) {
            return -1;
        }
        byte by = MINOR_AXES[n2];
        byte by2 = MINOR_AXES[n2 + 3];
        int n4 = nArray[n2] > 0 ? 1 : -1;
        double d = (double)nArray[by] / (double)nArray[n2];
        double d2 = (double)nArray[by2] / (double)nArray[n2];
        int[] nArray2 = new int[]{0, 0, 0};
        int n5 = nArray[n2] + n4;
        for (n = 0; n != n5; n += n4) {
            nArray2[n2] = from[n2] + n;
            nArray2[by] = MathHelper.floor((double)from[by] + (double)n * d);
            nArray2[by2] = MathHelper.floor((double)from[by2] + (double)n * d2);
            int n6 = this.world.getBlockId(nArray2[0], nArray2[1], nArray2[2]);
            if (n6 != 0 && n6 != 18) break;
        }
        if (n == n5) {
            return -1;
        }
        return Math.abs(n);
    }

    boolean canPlace() {
        int[] nArray = new int[]{this.origin[0], this.origin[1], this.origin[2]};
        int[] nArray2 = new int[]{this.origin[0], this.origin[1] + this.height - 1, this.origin[2]};
        int n = this.world.getBlockId(this.origin[0], this.origin[1] - 1, this.origin[2]);
        if (n != 2 && n != 3) {
            return false;
        }
        int n2 = this.tryBranch(nArray, nArray2);
        if (n2 == -1) {
            return true;
        }
        if (n2 < 6) {
            return false;
        }
        this.height = n2;
        return true;
    }

    public void prepare(double d0, double d1, double d2) {
        this.maxTrunkHeight = (int)(d0 * 12.0);
        if (d0 > 0.5) {
            this.foliageClusterHeight = 5;
        }
        this.branchLengthScale = d1;
        this.foliageDensity = d2;
    }

    public boolean generate(World world, Random random, int x, int y, int z) {
        this.world = world;
        long l = random.nextLong();
        this.random.setSeed(l);
        this.origin[0] = x;
        this.origin[1] = y;
        this.origin[2] = z;
        if (this.height == 0) {
            this.height = 5 + this.random.nextInt(this.maxTrunkHeight);
        }
        if (!this.canPlace()) {
            return false;
        }
        this.makeBranches();
        this.placeFoliage();
        this.PlaceTrunk();
        this.placeBranches();
        return true;
    }
}

