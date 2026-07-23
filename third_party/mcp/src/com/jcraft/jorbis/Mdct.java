/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Mdct {
    int n;
    int log2n;
    float[] trig;
    int[] bitrev;
    float scale;
    float[] _x = new float[1024];
    float[] _w = new float[1024];

    Mdct() {
    }

    void init(int i) {
        int n;
        this.bitrev = new int[i / 4];
        this.trig = new float[i + i / 4];
        this.log2n = (int)Math.rint(Math.log(i) / Math.log(2.0));
        this.n = i;
        int n2 = 0;
        int n3 = 1;
        int n4 = n2 + i / 2;
        int n5 = n4 + 1;
        int n6 = n4 + i / 2;
        int n7 = n6 + 1;
        for (n = 0; n < i / 4; ++n) {
            this.trig[n2 + n * 2] = (float)Math.cos(Math.PI / (double)i * (double)(4 * n));
            this.trig[n3 + n * 2] = (float)(-Math.sin(Math.PI / (double)i * (double)(4 * n)));
            this.trig[n4 + n * 2] = (float)Math.cos(Math.PI / (double)(2 * i) * (double)(2 * n + 1));
            this.trig[n5 + n * 2] = (float)Math.sin(Math.PI / (double)(2 * i) * (double)(2 * n + 1));
        }
        for (n = 0; n < i / 8; ++n) {
            this.trig[n6 + n * 2] = (float)Math.cos(Math.PI / (double)i * (double)(4 * n + 2));
            this.trig[n7 + n * 2] = (float)(-Math.sin(Math.PI / (double)i * (double)(4 * n + 2)));
        }
        n = (1 << this.log2n - 1) - 1;
        int n8 = 1 << this.log2n - 2;
        for (int j = 0; j < i / 8; ++j) {
            int n9 = 0;
            int n10 = 0;
            while (n8 >>> n10 != 0) {
                if ((n8 >>> n10 & j) != 0) {
                    n9 |= 1 << n10;
                }
                ++n10;
            }
            this.bitrev[j * 2] = ~n9 & n;
            this.bitrev[j * 2 + 1] = n9;
        }
        this.scale = 4.0f / (float)i;
    }

    void clear() {
    }

    void forward(float[] fs, float[] gs) {
    }

    synchronized void backward(float[] fs, float[] gs) {
        int n;
        if (this._x.length < this.n / 2) {
            this._x = new float[this.n / 2];
        }
        if (this._w.length < this.n / 2) {
            this._w = new float[this.n / 2];
        }
        float[] fArray = this._x;
        float[] fArray2 = this._w;
        int n2 = this.n >>> 1;
        int n3 = this.n >>> 2;
        int n4 = this.n >>> 3;
        int n5 = 1;
        int n6 = 0;
        int n7 = n2;
        for (n = 0; n < n4; ++n) {
            fArray[n6++] = -fs[n5 + 2] * this.trig[(n7 -= 2) + 1] - fs[n5] * this.trig[n7];
            fArray[n6++] = fs[n5] * this.trig[n7 + 1] - fs[n5 + 2] * this.trig[n7];
            n5 += 4;
        }
        n5 = n2 - 4;
        for (n = 0; n < n4; ++n) {
            fArray[n6++] = fs[n5] * this.trig[(n7 -= 2) + 1] + fs[n5 + 2] * this.trig[n7];
            fArray[n6++] = fs[n5] * this.trig[n7] - fs[n5 + 2] * this.trig[n7 + 1];
            n5 -= 4;
        }
        float[] fArray3 = this.mdct_kernel(fArray, fArray2, this.n, n2, n3, n4);
        n6 = 0;
        n7 = n2;
        n = n3;
        int n8 = n - 1;
        int n9 = n3 + n2;
        int n10 = n9 - 1;
        for (int i = 0; i < n3; ++i) {
            float f = fArray3[n6] * this.trig[n7 + 1] - fArray3[n6 + 1] * this.trig[n7];
            float f2 = -(fArray3[n6] * this.trig[n7] + fArray3[n6 + 1] * this.trig[n7 + 1]);
            gs[n] = -f;
            gs[n8] = f;
            gs[n9] = f2;
            gs[n10] = f2;
            ++n;
            --n8;
            ++n9;
            --n10;
            n6 += 2;
            n7 += 2;
        }
    }

    private float[] mdct_kernel(float[] fs, float[] gs, int i, int j, int k, int l) {
        float f;
        float f2;
        float f3;
        float f4;
        int n;
        int n2;
        int n3;
        int n4;
        int n5 = k;
        int n6 = 0;
        int n7 = k;
        int n8 = j;
        for (n4 = 0; n4 < k; ++n4) {
            float f5 = fs[n5] - fs[n6];
            gs[n7 + n4] = fs[n5++] + fs[n6++];
            float f6 = fs[n5] - fs[n6];
            gs[n4++] = f5 * this.trig[n8 -= 4] + f6 * this.trig[n8 + 1];
            gs[n4] = f6 * this.trig[n8] - f5 * this.trig[n8 + 1];
            gs[n7 + n4] = fs[n5++] + fs[n6++];
        }
        for (n4 = 0; n4 < this.log2n - 3; ++n4) {
            int n9 = i >>> n4 + 2;
            int n10 = 1 << n4 + 3;
            n3 = j - 2;
            n8 = 0;
            for (n2 = 0; n2 < n9 >>> 2; ++n2) {
                n = n3;
                n7 = n - (n9 >> 1);
                f4 = this.trig[n8];
                f3 = this.trig[n8 + 1];
                n3 -= 2;
                ++n9;
                for (int i2 = 0; i2 < 2 << n4; ++i2) {
                    f2 = gs[n] - gs[n7];
                    fs[n] = gs[n] + gs[n7];
                    f = gs[++n] - gs[++n7];
                    fs[n] = gs[n] + gs[n7];
                    fs[n7] = f * f4 - f2 * f3;
                    fs[n7 - 1] = f2 * f4 + f * f3;
                    n -= n9;
                    n7 -= n9;
                }
                --n9;
                n8 += n10;
            }
            float[] fArray = gs;
            gs = fs;
            fs = fArray;
        }
        n4 = i;
        int n11 = 0;
        int n12 = 0;
        n3 = j - 1;
        for (int i3 = 0; i3 < l; ++i3) {
            n2 = this.bitrev[n11++];
            n = this.bitrev[n11++];
            f4 = gs[n2] - gs[n + 1];
            f = gs[n2 - 1] + gs[n];
            f3 = gs[n2] + gs[n + 1];
            f2 = gs[n2 - 1] - gs[n];
            float f7 = f4 * this.trig[n4];
            float f8 = f * this.trig[n4++];
            float f9 = f4 * this.trig[n4];
            float f10 = f * this.trig[n4++];
            fs[n12++] = (f3 + f9 + f8) * 0.5f;
            fs[n3--] = (-f2 + f10 - f7) * 0.5f;
            fs[n12++] = (f2 + f10 - f7) * 0.5f;
            fs[n3--] = (f3 - f9 - f8) * 0.5f;
        }
        return fs;
    }
}

