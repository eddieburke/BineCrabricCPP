/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jorbis.Lookup;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Lsp {
    static final float M_PI = (float)Math.PI;

    Lsp() {
    }

    static void lsp_to_curve(float[] fs, int[] is, int i, int j, float[] gs, int k, float f, float g) {
        int n;
        float f2 = (float)Math.PI / (float)j;
        for (n = 0; n < k; ++n) {
            gs[n] = Lookup.coslook(gs[n]);
        }
        int n2 = k / 2 * 2;
        n = 0;
        while (n < i) {
            int n3;
            int n4 = is[n];
            float f3 = 0.70710677f;
            float f4 = 0.70710677f;
            float f5 = Lookup.coslook(f2 * (float)n4);
            for (n3 = 0; n3 < n2; n3 += 2) {
                f4 *= gs[n3] - f5;
                f3 *= gs[n3 + 1] - f5;
            }
            if ((k & 1) != 0) {
                f4 *= gs[k - 1] - f5;
                f4 *= f4;
                f3 *= f3 * (1.0f - f5 * f5);
            } else {
                f4 *= f4 * (1.0f + f5);
                f3 *= f3 * (1.0f - f5);
            }
            f4 = f3 + f4;
            n3 = Float.floatToIntBits(f4);
            int n5 = Integer.MAX_VALUE & n3;
            int n6 = 0;
            if (n5 < 2139095040 && n5 != 0) {
                if (n5 < 0x800000) {
                    f4 = (float)((double)f4 * 3.3554432E7);
                    n3 = Float.floatToIntBits(f4);
                    n5 = Integer.MAX_VALUE & n3;
                    n6 = -25;
                }
                n6 += (n5 >>> 23) - 126;
                n3 = n3 & 0x807FFFFF | 0x3F000000;
                f4 = Float.intBitsToFloat(n3);
            }
            f4 = Lookup.fromdBlook(f * Lookup.invsqlook(f4) * Lookup.invsq2explook(n6 + k) - g);
            do {
                int n7 = n++;
                fs[n7] = fs[n7] * f4;
            } while (n < i && is[n] == n4);
        }
    }
}

