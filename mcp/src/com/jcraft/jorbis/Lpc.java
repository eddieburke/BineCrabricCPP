/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jorbis.Drft;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Lpc {
    Drft fft = new Drft();
    int ln;
    int m;

    Lpc() {
    }

    static float lpc_from_data(float[] fs, float[] gs, int i, int j) {
        int n;
        float f;
        float[] fArray = new float[j + 1];
        int n2 = j + 1;
        while (n2-- != 0) {
            f = 0.0f;
            for (n = n2; n < i; ++n) {
                f += fs[n] * fs[n - n2];
            }
            fArray[n2] = f;
        }
        float f2 = fArray[0];
        for (n = 0; n < j; ++n) {
            f = -fArray[n + 1];
            if (f2 == 0.0f) {
                for (int k = 0; k < j; ++k) {
                    gs[k] = 0.0f;
                }
                return 0.0f;
            }
            for (n2 = 0; n2 < n; ++n2) {
                f -= gs[n2] * fArray[n - n2];
            }
            gs[n] = f /= f2;
            for (n2 = 0; n2 < n / 2; ++n2) {
                float f3 = gs[n2];
                int n3 = n2;
                gs[n3] = gs[n3] + f * gs[n - 1 - n2];
                int n4 = n - 1 - n2;
                gs[n4] = gs[n4] + f * f3;
            }
            if (n % 2 != 0) {
                int n5 = n2;
                gs[n5] = gs[n5] + gs[n2] * f;
            }
            f2 = (float)((double)f2 * (1.0 - (double)(f * f)));
        }
        return f2;
    }

    float lpc_from_curve(float[] fs, float[] gs) {
        int n;
        int n2 = this.ln;
        float[] fArray = new float[n2 + n2];
        float f = (float)(0.5 / (double)n2);
        for (n = 0; n < n2; ++n) {
            fArray[n * 2] = fs[n] * f;
            fArray[n * 2 + 1] = 0.0f;
        }
        fArray[n2 * 2 - 1] = fs[n2 - 1] * f;
        this.fft.backward(fArray);
        n = 0;
        int n3 = (n2 *= 2) / 2;
        while (n < n2 / 2) {
            float f2 = fArray[n];
            fArray[n++] = fArray[n3];
            fArray[n3++] = f2;
        }
        return Lpc.lpc_from_data(fArray, gs, n2, this.m);
    }

    void init(int i, int j) {
        this.ln = i;
        this.m = j;
        this.fft.init(i * 2);
    }

    void clear() {
        this.fft.clear();
    }

    static float FAST_HYPOT(float f, float g) {
        return (float)Math.sqrt(f * f + g * g);
    }

    void lpc_to_curve(float[] fs, float[] gs, float f) {
        int n;
        for (n = 0; n < this.ln * 2; ++n) {
            fs[n] = 0.0f;
        }
        if (f == 0.0f) {
            return;
        }
        for (n = 0; n < this.m; ++n) {
            fs[n * 2 + 1] = gs[n] / (4.0f * f);
            fs[n * 2 + 2] = -gs[n] / (4.0f * f);
        }
        this.fft.backward(fs);
        n = this.ln * 2;
        float f2 = (float)(1.0 / (double)f);
        fs[0] = (float)(1.0 / (double)(fs[0] * 2.0f + f2));
        for (int i = 1; i < this.ln; ++i) {
            float f3 = fs[i] + fs[n - i];
            float f4 = fs[i] - fs[n - i];
            float f5 = f3 + f2;
            fs[i] = (float)(1.0 / (double)Lpc.FAST_HYPOT(f5, f4));
        }
    }
}

