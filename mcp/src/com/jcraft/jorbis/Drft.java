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
class Drft {
    int n;
    float[] trigcache;
    int[] splitcache;
    static int[] ntryh = new int[]{4, 2, 3, 5};
    static float tpi = (float)Math.PI * 2;
    static float hsqt2 = 0.70710677f;
    static float taui = 0.8660254f;
    static float taur = -0.5f;
    static float sqrt2 = 1.4142135f;

    Drft() {
    }

    void backward(float[] fs) {
        if (this.n == 1) {
            return;
        }
        Drft.drftb1(this.n, fs, this.trigcache, this.trigcache, this.n, this.splitcache);
    }

    void init(int i) {
        this.n = i;
        this.trigcache = new float[3 * i];
        this.splitcache = new int[32];
        Drft.fdrffti(i, this.trigcache, this.splitcache);
    }

    void clear() {
        if (this.trigcache != null) {
            this.trigcache = null;
        }
        if (this.splitcache != null) {
            this.splitcache = null;
        }
    }

    /*
     * Enabled aggressive block sorting
     */
    static void drfti1(int i, float[] fs, int j, int[] is) {
        int n = 0;
        int n2 = -1;
        int n3 = i;
        int n4 = 0;
        int n5 = 101;
        while (true) {
            switch (n5) {
                case 101: {
                    n = ++n2 < 4 ? ntryh[n2] : (n += 2);
                }
                case 104: {
                    int n6;
                    int n7 = n3 / n;
                    int n8 = n3 - n * n7;
                    if (n8 != 0) {
                        n5 = 101;
                        break;
                    }
                    is[++n4 + 1] = n;
                    n3 = n7;
                    if (n != 2) {
                        n5 = 107;
                        break;
                    }
                    if (n4 == 1) {
                        n5 = 107;
                        break;
                    }
                    for (n6 = 1; n6 < n4; ++n6) {
                        int n9 = n4 - n6 + 1;
                        is[n9 + 1] = is[n9];
                    }
                    is[2] = 2;
                }
                case 107: {
                    int n6;
                    if (n3 != 1) {
                        n5 = 104;
                        break;
                    }
                    is[0] = i;
                    is[1] = n4;
                    float f = tpi / (float)i;
                    int n10 = 0;
                    int n11 = n4 - 1;
                    int n12 = 1;
                    if (n11 == 0) {
                        return;
                    }
                    for (int k = 0; k < n11; ++k) {
                        int n13 = is[k + 2];
                        int n14 = 0;
                        int n15 = n12 * n13;
                        int n16 = i / n15;
                        int n17 = n13 - 1;
                        for (n2 = 0; n2 < n17; n10 += n16, ++n2) {
                            n6 = n10;
                            float f2 = (float)(n14 += n12) * f;
                            float f3 = 0.0f;
                            for (int i2 = 2; i2 < n16; i2 += 2) {
                                float f4 = (f3 += 1.0f) * f2;
                                fs[j + n6++] = (float)Math.cos(f4);
                                fs[j + n6++] = (float)Math.sin(f4);
                            }
                        }
                        n12 = n15;
                    }
                    return;
                }
            }
        }
    }

    static void fdrffti(int i, float[] fs, int[] is) {
        if (i == 1) {
            return;
        }
        Drft.drfti1(i, fs, i, is);
    }

    static void dradf2(int i, int j, float[] fs, float[] gs, float[] hs, int k) {
        int n;
        int n2;
        int n3 = 0;
        int n4 = n2 = j * i;
        int n5 = i << 1;
        for (n = 0; n < j; ++n) {
            gs[n3 << 1] = fs[n3] + fs[n2];
            gs[(n3 << 1) + n5 - 1] = fs[n3] - fs[n2];
            n3 += i;
            n2 += i;
        }
        if (i < 2) {
            return;
        }
        if (i != 2) {
            n3 = 0;
            n2 = n4;
            for (n = 0; n < j; ++n) {
                n5 = n2;
                int n6 = (n3 << 1) + (i << 1);
                int n7 = n3;
                int n8 = n3 + n3;
                for (int i2 = 2; i2 < i; i2 += 2) {
                    float f = hs[k + i2 - 2] * fs[(n5 += 2) - 1] + hs[k + i2 - 1] * fs[n5];
                    float f2 = hs[k + i2 - 2] * fs[n5] - hs[k + i2 - 1] * fs[n5 - 1];
                    gs[n8 += 2] = fs[n7 += 2] + f2;
                    gs[n6 -= 2] = f2 - fs[n7];
                    gs[n8 - 1] = fs[n7 - 1] + f;
                    gs[n6 - 1] = fs[n7 - 1] - f;
                }
                n3 += i;
                n2 += i;
            }
            if (i % 2 == 1) {
                return;
            }
        }
        n3 = i;
        n5 = n2 = n3 - 1;
        n2 += n4;
        for (n = 0; n < j; ++n) {
            gs[n3] = -fs[n2];
            gs[n3 - 1] = fs[n5];
            n3 += i << 1;
            n2 += i;
            n5 += i;
        }
    }

    static void dradf4(int i, int j, float[] fs, float[] gs, float[] hs, int k, float[] is, int l, float[] js, int m) {
        float f;
        int n;
        int n2;
        float f2;
        float f3;
        int n3;
        int n4;
        int n5 = n4 = j * i;
        int n6 = n5 << 1;
        int n7 = n5 + (n5 << 1);
        int n8 = 0;
        for (n3 = 0; n3 < j; ++n3) {
            f3 = fs[n5] + fs[n7];
            f2 = fs[n8] + fs[n6];
            n2 = n8 << 2;
            gs[n2] = f3 + f2;
            gs[(i << 2) + n2 - 1] = f2 - f3;
            gs[(n2 += i << 1) - 1] = fs[n8] - fs[n6];
            gs[n2] = fs[n7] - fs[n5];
            n5 += i;
            n7 += i;
            n8 += i;
            n6 += i;
        }
        if (i < 2) {
            return;
        }
        if (i != 2) {
            n5 = 0;
            for (n3 = 0; n3 < j; ++n3) {
                n7 = n5;
                n6 = n5 << 2;
                n = i << 1;
                n2 = n + n6;
                for (int i2 = 2; i2 < i; i2 += 2) {
                    n8 = n7 += 2;
                    n6 += 2;
                    n2 -= 2;
                    float f4 = hs[k + i2 - 2] * fs[(n8 += n4) - 1] + hs[k + i2 - 1] * fs[n8];
                    float f5 = hs[k + i2 - 2] * fs[n8] - hs[k + i2 - 1] * fs[n8 - 1];
                    float f6 = is[l + i2 - 2] * fs[(n8 += n4) - 1] + is[l + i2 - 1] * fs[n8];
                    float f7 = is[l + i2 - 2] * fs[n8] - is[l + i2 - 1] * fs[n8 - 1];
                    float f8 = js[m + i2 - 2] * fs[(n8 += n4) - 1] + js[m + i2 - 1] * fs[n8];
                    float f9 = js[m + i2 - 2] * fs[n8] - js[m + i2 - 1] * fs[n8 - 1];
                    f3 = f4 + f8;
                    float f10 = f8 - f4;
                    f = f5 + f9;
                    float f11 = f5 - f9;
                    float f12 = fs[n7] + f7;
                    float f13 = fs[n7] - f7;
                    f2 = fs[n7 - 1] + f6;
                    float f14 = fs[n7 - 1] - f6;
                    gs[n6 - 1] = f3 + f2;
                    gs[n6] = f + f12;
                    gs[n2 - 1] = f14 - f11;
                    gs[n2] = f10 - f13;
                    gs[n6 + n - 1] = f11 + f14;
                    gs[n6 + n] = f10 + f13;
                    gs[n2 + n - 1] = f2 - f3;
                    gs[n2 + n] = f - f12;
                }
                n5 += i;
            }
            if ((i & 1) != 0) {
                return;
            }
        }
        n5 = n4 + i - 1;
        n7 = n5 + (n4 << 1);
        n8 = i << 2;
        n6 = i;
        n2 = i << 1;
        n = i;
        for (n3 = 0; n3 < j; ++n3) {
            f = -hsqt2 * (fs[n5] + fs[n7]);
            f3 = hsqt2 * (fs[n5] - fs[n7]);
            gs[n6 - 1] = f3 + fs[n - 1];
            gs[n6 + n2 - 1] = fs[n - 1] - f3;
            gs[n6] = f - fs[n5 + n4];
            gs[n6 + n2] = f + fs[n5 + n4];
            n5 += i;
            n7 += i;
            n6 += n8;
            n += i;
        }
    }

    /*
     * Unable to fully structure code
     */
    static void dradfg(int i, int j, int k, int l, float[] fs, float[] gs, float[] hs, float[] is, float[] js, float[] ks, int m) {
        var22_11 = 0;
        var38_12 = 0.0f;
        var40_13 = 0.0f;
        var39_14 = Drft.tpi / (float)j;
        var38_12 = (float)Math.cos(var39_14);
        var40_13 = (float)Math.sin(var39_14);
        var12_15 = j + 1 >> 1;
        var44_16 = j;
        var43_17 = i;
        var37_18 = i - 1 >> 1;
        var20_19 = k * i;
        var30_20 = j * i;
        var45_21 = 100;
        block7: while (true) {
            switch (var45_21) {
                case 101: {
                    if (i != 1) ** GOTO lbl20
                    var45_21 = 119;
                    ** GOTO lbl258
lbl20:
                    // 2 sources

                    for (var18_27 = 0; var18_27 < l; ++var18_27) {
                        js[var18_27] = hs[var18_27];
                    }
                    var21_29 = 0;
                    for (var14_24 = 1; var14_24 < j; ++var14_24) {
                        var22_11 = var21_29 += var20_19;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            is[var22_11] = gs[var22_11];
                            var22_11 += i;
                        }
                    }
                    var19_28 = -i;
                    var21_29 = 0;
                    if (var37_18 > k) {
                        for (var14_24 = 1; var14_24 < j; ++var14_24) {
                            var19_28 += i;
                            var22_11 = -i + (var21_29 += var20_19);
                            for (var15_25 = 0; var15_25 < k; ++var15_25) {
                                var11_22 = var19_28 - 1;
                                var23_30 = var22_11 += i;
                                for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                                    is[(var23_30 += 2) - 1] = ks[m + (var11_22 += 2) - 1] * gs[var23_30 - 1] + ks[m + var11_22] * gs[var23_30];
                                    is[var23_30] = ks[m + var11_22 - 1] * gs[var23_30] - ks[m + var11_22] * gs[var23_30 - 1];
                                }
                            }
                        }
                    } else {
                        for (var14_24 = 1; var14_24 < j; ++var14_24) {
                            var11_22 = (var19_28 += i) - 1;
                            var22_11 = var21_29 += var20_19;
                            for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                                var11_22 += 2;
                                var23_30 = var22_11 += 2;
                                for (var15_25 = 0; var15_25 < k; ++var15_25) {
                                    is[var23_30 - 1] = ks[m + var11_22 - 1] * gs[var23_30 - 1] + ks[m + var11_22] * gs[var23_30];
                                    is[var23_30] = ks[m + var11_22 - 1] * gs[var23_30] - ks[m + var11_22] * gs[var23_30 - 1];
                                    var23_30 += i;
                                }
                            }
                        }
                    }
                    var21_29 = 0;
                    var22_11 = var44_16 * var20_19;
                    if (var37_18 < k) {
                        for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                            var23_30 = var21_29 += var20_19;
                            var24_31 = var22_11 -= var20_19;
                            for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                                var25_32 = (var23_30 += 2) - i;
                                var26_33 = (var24_31 += 2) - i;
                                for (var15_25 = 0; var15_25 < k; ++var15_25) {
                                    gs[(var25_32 += i) - 1] = is[var25_32 - 1] + is[(var26_33 += i) - 1];
                                    gs[var26_33 - 1] = is[var25_32] - is[var26_33];
                                    gs[var25_32] = is[var25_32] + is[var26_33];
                                    gs[var26_33] = is[var26_33 - 1] - is[var25_32 - 1];
                                }
                            }
                        }
                    } else {
                        for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                            var23_30 = var21_29 += var20_19;
                            var24_31 = var22_11 -= var20_19;
                            for (var15_25 = 0; var15_25 < k; ++var15_25) {
                                var25_32 = var23_30;
                                var26_33 = var24_31;
                                for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                                    gs[(var25_32 += 2) - 1] = is[var25_32 - 1] + is[(var26_33 += 2) - 1];
                                    gs[var26_33 - 1] = is[var25_32] - is[var26_33];
                                    gs[var25_32] = is[var25_32] + is[var26_33];
                                    gs[var26_33] = is[var26_33 - 1] - is[var25_32 - 1];
                                }
                                var23_30 += i;
                                var24_31 += i;
                            }
                        }
                    }
                }
                case 119: {
                    for (var18_27 = 0; var18_27 < l; ++var18_27) {
                        hs[var18_27] = js[var18_27];
                    }
                    var21_29 = 0;
                    var22_11 = var44_16 * l;
                    for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                        var23_30 = (var21_29 += var20_19) - i;
                        var24_31 = (var22_11 -= var20_19) - i;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            gs[var23_30 += i] = is[var23_30] + is[var24_31 += i];
                            gs[var24_31] = is[var24_31] - is[var23_30];
                        }
                    }
                    var34_40 = 1.0f;
                    var32_38 = 0.0f;
                    var21_29 = 0;
                    var22_11 = var44_16 * l;
                    var23_30 = (j - 1) * l;
                    for (var16_26 = 1; var16_26 < var12_15; ++var16_26) {
                        var41_43 = var38_12 * var34_40 - var40_13 * var32_38;
                        var32_38 = var38_12 * var32_38 + var40_13 * var34_40;
                        var34_40 = var41_43;
                        var24_31 = var21_29 += l;
                        var25_32 = var22_11 -= l;
                        var26_33 = var23_30;
                        var27_34 = l;
                        for (var18_27 = 0; var18_27 < l; ++var18_27) {
                            js[var24_31++] = hs[var18_27] + var34_40 * hs[var27_34++];
                            js[var25_32++] = var32_38 * hs[var26_33++];
                        }
                        var31_37 = var34_40;
                        var36_42 = var32_38;
                        var35_41 = var34_40;
                        var33_39 = var32_38;
                        var24_31 = l;
                        var25_32 = (var44_16 - 1) * l;
                        for (var14_24 = 2; var14_24 < var12_15; ++var14_24) {
                            var24_31 += l;
                            var25_32 -= l;
                            var42_44 = var31_37 * var35_41 - var36_42 * var33_39;
                            var33_39 = var31_37 * var33_39 + var36_42 * var35_41;
                            var35_41 = var42_44;
                            var26_33 = var21_29;
                            var27_34 = var22_11;
                            var28_35 = var24_31;
                            var29_36 = var25_32;
                            for (var18_27 = 0; var18_27 < l; ++var18_27) {
                                v0 = var26_33++;
                                js[v0] = js[v0] + var35_41 * hs[var28_35++];
                                v1 = var27_34++;
                                js[v1] = js[v1] + var33_39 * hs[var29_36++];
                            }
                        }
                    }
                    var21_29 = 0;
                    for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                        var22_11 = var21_29 += l;
                        var18_27 = 0;
                        while (var18_27 < l) {
                            v2 = var18_27++;
                            js[v2] = js[v2] + hs[var22_11++];
                        }
                    }
                    if (i < k) {
                        var45_21 = 132;
                    } else {
                        var21_29 = 0;
                        var22_11 = 0;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            var23_30 = var21_29;
                            var24_31 = var22_11;
                            for (var13_23 = 0; var13_23 < i; ++var13_23) {
                                fs[var24_31++] = is[var23_30++];
                            }
                            var21_29 += i;
                            var22_11 += var30_20;
                        }
                        var45_21 = 135;
                    }
                    ** GOTO lbl258
                }
                case 132: {
                    for (var13_23 = 0; var13_23 < i; ++var13_23) {
                        var21_29 = var13_23;
                        var22_11 = var13_23;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            fs[var22_11] = is[var21_29];
                            var21_29 += i;
                            var22_11 += var30_20;
                        }
                    }
                }
                case 135: {
                    var21_29 = 0;
                    var22_11 = i << 1;
                    var23_30 = 0;
                    var24_31 = var44_16 * var20_19;
                    for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                        var25_32 = var21_29 += var22_11;
                        var26_33 = var23_30 += var20_19;
                        var27_34 = var24_31 -= var20_19;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            fs[var25_32 - 1] = is[var26_33];
                            fs[var25_32] = is[var27_34];
                            var25_32 += var30_20;
                            var26_33 += i;
                            var27_34 += i;
                        }
                    }
                    if (i == 1) {
                        return;
                    }
                    if (var37_18 >= k) ** GOTO lbl206
                    var45_21 = 141;
                    ** GOTO lbl258
lbl206:
                    // 1 sources

                    var21_29 = -i;
                    var23_30 = 0;
                    var24_31 = 0;
                    var25_32 = var44_16 * var20_19;
                    for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                        var26_33 = var21_29 += var22_11;
                        var27_34 = var23_30 += var22_11;
                        var28_35 = var24_31 += var20_19;
                        var29_36 = var25_32 -= var20_19;
                        for (var15_25 = 0; var15_25 < k; ++var15_25) {
                            for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                                var17_45 = var43_17 - var13_23;
                                fs[var13_23 + var27_34 - 1] = is[var13_23 + var28_35 - 1] + is[var13_23 + var29_36 - 1];
                                fs[var17_45 + var26_33 - 1] = is[var13_23 + var28_35 - 1] - is[var13_23 + var29_36 - 1];
                                fs[var13_23 + var27_34] = is[var13_23 + var28_35] + is[var13_23 + var29_36];
                                fs[var17_45 + var26_33] = is[var13_23 + var29_36] - is[var13_23 + var28_35];
                            }
                            var26_33 += var30_20;
                            var27_34 += var30_20;
                            var28_35 += i;
                            var29_36 += i;
                        }
                    }
                    return;
                }
                case 141: {
                    var21_29 = -i;
                    var23_30 = 0;
                    var24_31 = 0;
                    var25_32 = var44_16 * var20_19;
                    for (var14_24 = 1; var14_24 < var12_15; ++var14_24) {
                        var21_29 += var22_11;
                        var23_30 += var22_11;
                        var24_31 += var20_19;
                        var25_32 -= var20_19;
                        for (var13_23 = 2; var13_23 < i; var13_23 += 2) {
                            var26_33 = var43_17 + var21_29 - var13_23;
                            var27_34 = var13_23 + var23_30;
                            var28_35 = var13_23 + var24_31;
                            var29_36 = var13_23 + var25_32;
                            for (var15_25 = 0; var15_25 < k; ++var15_25) {
                                fs[var27_34 - 1] = is[var28_35 - 1] + is[var29_36 - 1];
                                fs[var26_33 - 1] = is[var28_35 - 1] - is[var29_36 - 1];
                                fs[var27_34] = is[var28_35] + is[var29_36];
                                fs[var26_33] = is[var29_36] - is[var28_35];
                                var26_33 += var30_20;
                                var27_34 += var30_20;
                                var28_35 += i;
                                var29_36 += i;
                            }
                        }
                    }
                    break block7;
                }
lbl258:
                // 5 sources

                default: {
                    continue block7;
                }
            }
            break;
        }
    }

    /*
     * Enabled aggressive block sorting
     */
    static void drftf1(int i, float[] fs, float[] gs, float[] hs, int[] is) {
        int n = is[1];
        int n2 = 1;
        int n3 = i;
        int n4 = i;
        int n5 = 0;
        while (true) {
            int n6;
            int n7;
            int n8;
            int n9;
            if (n5 < n) {
                int n10 = n - n5;
                n9 = is[n10 + 1];
                n8 = n3 / n9;
                n7 = i / n3;
                n6 = n7 * n8;
                n4 -= (n9 - 1) * n7;
            } else {
                if (n2 == 1) {
                    return;
                }
                int n11 = 0;
                while (true) {
                    if (n11 >= i) {
                        return;
                    }
                    fs[n11] = gs[n11];
                    ++n11;
                }
            }
            n2 = 1 - n2;
            int n12 = 100;
            block10: while (true) {
                switch (n12) {
                    case 100: {
                        if (n9 != 4) {
                            n12 = 102;
                            break;
                        }
                        int n13 = n4 + n7;
                        int n14 = n13 + n7;
                        if (n2 != 0) {
                            Drft.dradf4(n7, n8, gs, fs, hs, n4 - 1, hs, n13 - 1, hs, n14 - 1);
                        } else {
                            Drft.dradf4(n7, n8, fs, gs, hs, n4 - 1, hs, n13 - 1, hs, n14 - 1);
                        }
                        n12 = 110;
                        break;
                    }
                    case 102: {
                        if (n9 != 2) {
                            n12 = 104;
                            break;
                        }
                        if (n2 != 0) {
                            n12 = 103;
                            break;
                        }
                        Drft.dradf2(n7, n8, fs, gs, hs, n4 - 1);
                        n12 = 110;
                        break;
                    }
                    case 103: {
                        Drft.dradf2(n7, n8, gs, fs, hs, n4 - 1);
                    }
                    case 104: {
                        if (n7 == 1) {
                            n2 = 1 - n2;
                        }
                        if (n2 != 0) {
                            n12 = 109;
                            break;
                        }
                        Drft.dradfg(n7, n9, n8, n6, fs, fs, fs, gs, gs, hs, n4 - 1);
                        n2 = 1;
                        n12 = 110;
                        break;
                    }
                    case 109: {
                        Drft.dradfg(n7, n9, n8, n6, gs, gs, gs, fs, fs, hs, n4 - 1);
                        n2 = 0;
                    }
                    case 110: {
                        n3 = n8;
                        break block10;
                    }
                }
            }
            ++n5;
        }
    }

    static void dradb2(int i, int j, float[] fs, float[] gs, float[] hs, int k) {
        int n;
        int n2 = j * i;
        int n3 = 0;
        int n4 = 0;
        int n5 = (i << 1) - 1;
        for (n = 0; n < j; ++n) {
            gs[n3] = fs[n4] + fs[n5 + n4];
            gs[n3 + n2] = fs[n4] - fs[n5 + n4];
            n4 = (n3 += i) << 1;
        }
        if (i < 2) {
            return;
        }
        if (i != 2) {
            n3 = 0;
            n4 = 0;
            for (n = 0; n < j; ++n) {
                n5 = n3;
                int n6 = n4;
                int n7 = n6 + (i << 1);
                int n8 = n2 + n3;
                for (int i2 = 2; i2 < i; i2 += 2) {
                    gs[(n5 += 2) - 1] = fs[(n6 += 2) - 1] + fs[(n7 -= 2) - 1];
                    float f = fs[n6 - 1] - fs[n7 - 1];
                    gs[n5] = fs[n6] - fs[n7];
                    float f2 = fs[n6] + fs[n7];
                    gs[(n8 += 2) - 1] = hs[k + i2 - 2] * f - hs[k + i2 - 1] * f2;
                    gs[n8] = hs[k + i2 - 2] * f2 + hs[k + i2 - 1] * f;
                }
                n4 = (n3 += i) << 1;
            }
            if (i % 2 == 1) {
                return;
            }
        }
        n3 = i - 1;
        n4 = i - 1;
        for (n = 0; n < j; ++n) {
            gs[n3] = fs[n4] + fs[n4];
            gs[n3 + n2] = -(fs[n4 + 1] + fs[n4 + 1]);
            n3 += i;
            n4 += i << 1;
        }
    }

    static void dradb3(int i, int j, float[] fs, float[] gs, float[] hs, int k, float[] is, int l) {
        float f;
        float f2;
        float f3;
        int n;
        int n2 = j * i;
        int n3 = 0;
        int n4 = n2 << 1;
        int n5 = i << 1;
        int n6 = i + (i << 1);
        int n7 = 0;
        for (n = 0; n < j; ++n) {
            f3 = fs[n5 - 1] + fs[n5 - 1];
            f2 = fs[n7] + taur * f3;
            gs[n3] = fs[n7] + f3;
            f = taui * (fs[n5] + fs[n5]);
            gs[n3 + n2] = f2 - f;
            gs[n3 + n4] = f2 + f;
            n3 += i;
            n5 += n6;
            n7 += n6;
        }
        if (i == 1) {
            return;
        }
        n3 = 0;
        n5 = i << 1;
        for (n = 0; n < j; ++n) {
            int n8 = n3 + (n3 << 1);
            int n9 = n7 = n8 + n5;
            int n10 = n3;
            int n11 = n3 + n2;
            int n12 = n11 + n2;
            for (int i2 = 2; i2 < i; i2 += 2) {
                n11 += 2;
                n12 += 2;
                f3 = fs[(n7 += 2) - 1] + fs[(n9 -= 2) - 1];
                f2 = fs[(n8 += 2) - 1] + taur * f3;
                gs[(n10 += 2) - 1] = fs[n8 - 1] + f3;
                float f4 = fs[n7] - fs[n9];
                float f5 = fs[n8] + taur * f4;
                gs[n10] = fs[n8] + f4;
                float f6 = taui * (fs[n7 - 1] - fs[n9 - 1]);
                f = taui * (fs[n7] + fs[n9]);
                float f7 = f2 - f;
                float f8 = f2 + f;
                float f9 = f5 + f6;
                float f10 = f5 - f6;
                gs[n11 - 1] = hs[k + i2 - 2] * f7 - hs[k + i2 - 1] * f9;
                gs[n11] = hs[k + i2 - 2] * f9 + hs[k + i2 - 1] * f7;
                gs[n12 - 1] = is[l + i2 - 2] * f8 - is[l + i2 - 1] * f10;
                gs[n12] = is[l + i2 - 2] * f10 + is[l + i2 - 1] * f8;
            }
            n3 += i;
        }
    }

    static void dradb4(int i, int j, float[] fs, float[] gs, float[] hs, int k, float[] is, int l, float[] js, int m) {
        float f;
        float f2;
        float f3;
        float f4;
        float f5;
        float f6;
        int n;
        int n2;
        int n3;
        int n4 = j * i;
        int n5 = 0;
        int n6 = i << 2;
        int n7 = 0;
        int n8 = i << 1;
        for (n3 = 0; n3 < j; ++n3) {
            n2 = n7 + n8;
            n = n5;
            f6 = fs[n2 - 1] + fs[n2 - 1];
            f5 = fs[n2] + fs[n2];
            f4 = fs[n7] - fs[(n2 += n8) - 1];
            f3 = fs[n7] + fs[n2 - 1];
            gs[n] = f3 + f6;
            gs[n += n4] = f4 - f5;
            gs[n += n4] = f3 - f6;
            gs[n += n4] = f4 + f5;
            n5 += i;
            n7 += n6;
        }
        if (i < 2) {
            return;
        }
        if (i != 2) {
            n5 = 0;
            for (n3 = 0; n3 < j; ++n3) {
                n6 = n5 << 2;
                n2 = n7 = n6 + n8;
                n = n7 + n8;
                int n9 = n5;
                for (int i2 = 2; i2 < i; i2 += 2) {
                    n9 += 2;
                    f2 = fs[n6 += 2] + fs[n -= 2];
                    f = fs[n6] - fs[n];
                    float f7 = fs[n7 += 2] - fs[n2 -= 2];
                    f5 = fs[n7] + fs[n2];
                    f4 = fs[n6 - 1] - fs[n - 1];
                    f3 = fs[n6 - 1] + fs[n - 1];
                    float f8 = fs[n7 - 1] - fs[n2 - 1];
                    f6 = fs[n7 - 1] + fs[n2 - 1];
                    gs[n9 - 1] = f3 + f6;
                    float f9 = f3 - f6;
                    gs[n9] = f + f7;
                    float f10 = f - f7;
                    float f11 = f4 - f5;
                    float f12 = f4 + f5;
                    float f13 = f2 + f8;
                    float f14 = f2 - f8;
                    int n10 = n9 + n4;
                    gs[n10 - 1] = hs[k + i2 - 2] * f11 - hs[k + i2 - 1] * f13;
                    gs[n10] = hs[k + i2 - 2] * f13 + hs[k + i2 - 1] * f11;
                    gs[(n10 += n4) - 1] = is[l + i2 - 2] * f9 - is[l + i2 - 1] * f10;
                    gs[n10] = is[l + i2 - 2] * f10 + is[l + i2 - 1] * f9;
                    gs[(n10 += n4) - 1] = js[m + i2 - 2] * f12 - js[m + i2 - 1] * f14;
                    gs[n10] = js[m + i2 - 2] * f14 + js[m + i2 - 1] * f12;
                }
                n5 += i;
            }
            if (i % 2 == 1) {
                return;
            }
        }
        n5 = i;
        n6 = i << 2;
        n7 = i - 1;
        n2 = i + (i << 1);
        for (n3 = 0; n3 < j; ++n3) {
            n = n7;
            f2 = fs[n5] + fs[n2];
            f = fs[n2] - fs[n5];
            f4 = fs[n5 - 1] - fs[n2 - 1];
            f3 = fs[n5 - 1] + fs[n2 - 1];
            gs[n] = f3 + f3;
            gs[n += n4] = sqrt2 * (f4 - f2);
            gs[n += n4] = f + f;
            gs[n += n4] = -sqrt2 * (f4 + f2);
            n7 += i;
            n5 += n6;
            n2 += n6;
        }
    }

    /*
     * Unable to fully structure code
     */
    static void dradbg(int i, int j, int k, int l, float[] fs, float[] gs, float[] hs, float[] is, float[] js, float[] ks, int m) {
        var12_11 = 0;
        var19_12 = 0;
        var29_13 = 0;
        var38_14 = 0;
        var39_15 = 0.0f;
        var41_16 = 0.0f;
        var44_17 = 0;
        var45_18 = 100;
        block10: while (true) {
            switch (var45_18) {
                case 100: {
                    var29_13 = j * i;
                    var19_12 = k * i;
                    var40_41 = Drft.tpi / (float)j;
                    var39_15 = (float)Math.cos(var40_41);
                    var41_16 = (float)Math.sin(var40_41);
                    var38_14 = i - 1 >>> 1;
                    var44_17 = j;
                    var12_11 = j + 1 >>> 1;
                    if (i < k) {
                        var45_18 = 103;
                    } else {
                        var20_24 = 0;
                        var21_25 = 0;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            var22_26 = var20_24;
                            var23_27 = var21_25;
                            for (var13_19 = 0; var13_19 < i; ++var13_19) {
                                is[var22_26] = fs[var23_27];
                                ++var22_26;
                                ++var23_27;
                            }
                            var20_24 += i;
                            var21_25 += var29_13;
                        }
                        var45_18 = 106;
                    }
                    ** GOTO lbl283
                }
                case 103: {
                    var20_24 = 0;
                    for (var13_19 = 0; var13_19 < i; ++var13_19) {
                        var21_25 = var20_24;
                        var22_26 = var20_24;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            is[var21_25] = fs[var22_26];
                            var21_25 += i;
                            var22_26 += var29_13;
                        }
                        ++var20_24;
                    }
                }
                case 106: {
                    var20_24 = 0;
                    var21_25 = var44_17 * var19_12;
                    var26_30 = var24_28 = i << 1;
                    for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                        var22_26 = var20_24 += var19_12;
                        var23_27 = var21_25 -= var19_12;
                        var25_29 = var24_28;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            is[var22_26] = fs[var25_29 - 1] + fs[var25_29 - 1];
                            is[var23_27] = fs[var25_29] + fs[var25_29];
                            var22_26 += i;
                            var23_27 += i;
                            var25_29 += var29_13;
                        }
                        var24_28 += var26_30;
                    }
                    if (i == 1) {
                        var45_18 = 116;
                    } else if (var38_14 < k) {
                        var45_18 = 112;
                    } else {
                        var20_24 = 0;
                        var21_25 = var44_17 * var19_12;
                        var26_30 = 0;
                        for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                            var22_26 = var20_24 += var19_12;
                            var23_27 = var21_25 -= var19_12;
                            var27_31 = var26_30 += i << 1;
                            for (var15_21 = 0; var15_21 < k; ++var15_21) {
                                var24_28 = var22_26;
                                var25_29 = var23_27;
                                var28_32 = var27_31;
                                var30_33 = var27_31;
                                for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                                    is[(var24_28 += 2) - 1] = fs[(var28_32 += 2) - 1] + fs[(var30_33 -= 2) - 1];
                                    is[(var25_29 += 2) - 1] = fs[var28_32 - 1] - fs[var30_33 - 1];
                                    is[var24_28] = fs[var28_32] - fs[var30_33];
                                    is[var25_29] = fs[var28_32] + fs[var30_33];
                                }
                                var22_26 += i;
                                var23_27 += i;
                                var27_31 += var29_13;
                            }
                        }
                        var45_18 = 116;
                    }
                    ** GOTO lbl283
                }
                case 112: {
                    var20_24 = 0;
                    var21_25 = var44_17 * var19_12;
                    var26_30 = 0;
                    for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                        var22_26 = var20_24 += var19_12;
                        var23_27 = var21_25 -= var19_12;
                        var27_31 = var26_30 += i << 1;
                        var28_32 = var26_30;
                        for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                            var24_28 = var22_26 += 2;
                            var25_29 = var23_27 += 2;
                            var30_33 = var27_31 += 2;
                            var31_34 = var28_32 -= 2;
                            for (var15_21 = 0; var15_21 < k; ++var15_21) {
                                is[var24_28 - 1] = fs[var30_33 - 1] + fs[var31_34 - 1];
                                is[var25_29 - 1] = fs[var30_33 - 1] - fs[var31_34 - 1];
                                is[var24_28] = fs[var30_33] - fs[var31_34];
                                is[var25_29] = fs[var30_33] + fs[var31_34];
                                var24_28 += i;
                                var25_29 += i;
                                var30_33 += var29_13;
                                var31_34 += var29_13;
                            }
                        }
                    }
                }
                case 116: {
                    var35_38 = 1.0f;
                    var33_36 = 0.0f;
                    var20_24 = 0;
                    var28_32 = var21_25 = var44_17 * l;
                    var22_26 = (j - 1) * l;
                    for (var16_22 = 1; var16_22 < var12_11; ++var16_22) {
                        var42_42 = var39_15 * var35_38 - var41_16 * var33_36;
                        var33_36 = var39_15 * var33_36 + var41_16 * var35_38;
                        var35_38 = var42_42;
                        var23_27 = var20_24 += l;
                        var24_28 = var21_25 -= l;
                        var25_29 = 0;
                        var26_30 = l;
                        var27_31 = var22_26;
                        for (var17_23 = 0; var17_23 < l; ++var17_23) {
                            hs[var23_27++] = js[var25_29++] + var35_38 * js[var26_30++];
                            hs[var24_28++] = var33_36 * js[var27_31++];
                        }
                        var32_35 = var35_38;
                        var37_40 = var33_36;
                        var36_39 = var35_38;
                        var34_37 = var33_36;
                        var25_29 = l;
                        var26_30 = var28_32 - l;
                        for (var14_20 = 2; var14_20 < var12_11; ++var14_20) {
                            var25_29 += l;
                            var26_30 -= l;
                            var43_43 = var32_35 * var36_39 - var37_40 * var34_37;
                            var34_37 = var32_35 * var34_37 + var37_40 * var36_39;
                            var36_39 = var43_43;
                            var23_27 = var20_24;
                            var24_28 = var21_25;
                            var30_33 = var25_29;
                            var31_34 = var26_30;
                            for (var17_23 = 0; var17_23 < l; ++var17_23) {
                                v0 = var23_27++;
                                hs[v0] = hs[v0] + var36_39 * js[var30_33++];
                                v1 = var24_28++;
                                hs[v1] = hs[v1] + var34_37 * js[var31_34++];
                            }
                        }
                    }
                    var20_24 = 0;
                    for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                        var21_25 = var20_24 += l;
                        var17_23 = 0;
                        while (var17_23 < l) {
                            v2 = var17_23++;
                            js[v2] = js[v2] + js[var21_25++];
                        }
                    }
                    var20_24 = 0;
                    var21_25 = var44_17 * var19_12;
                    for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                        var22_26 = var20_24 += var19_12;
                        var23_27 = var21_25 -= var19_12;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            is[var22_26] = gs[var22_26] - gs[var23_27];
                            is[var23_27] = gs[var22_26] + gs[var23_27];
                            var22_26 += i;
                            var23_27 += i;
                        }
                    }
                    if (i == 1) {
                        var45_18 = 132;
                    } else if (var38_14 < k) {
                        var45_18 = 128;
                    } else {
                        var20_24 = 0;
                        var21_25 = var44_17 * var19_12;
                        for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                            var22_26 = var20_24 += var19_12;
                            var23_27 = var21_25 -= var19_12;
                            for (var15_21 = 0; var15_21 < k; ++var15_21) {
                                var24_28 = var22_26;
                                var25_29 = var23_27;
                                for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                                    is[(var24_28 += 2) - 1] = gs[var24_28 - 1] - gs[var25_29 += 2];
                                    is[var25_29 - 1] = gs[var24_28 - 1] + gs[var25_29];
                                    is[var24_28] = gs[var24_28] + gs[var25_29 - 1];
                                    is[var25_29] = gs[var24_28] - gs[var25_29 - 1];
                                }
                                var22_26 += i;
                                var23_27 += i;
                            }
                        }
                        var45_18 = 132;
                    }
                    ** GOTO lbl283
                }
                case 128: {
                    var20_24 = 0;
                    var21_25 = var44_17 * var19_12;
                    for (var14_20 = 1; var14_20 < var12_11; ++var14_20) {
                        var22_26 = var20_24 += var19_12;
                        var23_27 = var21_25 -= var19_12;
                        for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                            var24_28 = var22_26 += 2;
                            var25_29 = var23_27 += 2;
                            for (var15_21 = 0; var15_21 < k; ++var15_21) {
                                is[var24_28 - 1] = gs[var24_28 - 1] - gs[var25_29];
                                is[var25_29 - 1] = gs[var24_28 - 1] + gs[var25_29];
                                is[var24_28] = gs[var24_28] + gs[var25_29 - 1];
                                is[var25_29] = gs[var24_28] - gs[var25_29 - 1];
                                var24_28 += i;
                                var25_29 += i;
                            }
                        }
                    }
                }
                case 132: {
                    if (i == 1) {
                        return;
                    }
                    for (var17_23 = 0; var17_23 < l; ++var17_23) {
                        hs[var17_23] = js[var17_23];
                    }
                    var20_24 = 0;
                    for (var14_20 = 1; var14_20 < j; ++var14_20) {
                        var21_25 = var20_24 += var19_12;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            gs[var21_25] = is[var21_25];
                            var21_25 += i;
                        }
                    }
                    if (var38_14 <= k) ** GOTO lbl250
                    var45_18 = 139;
                    ** GOTO lbl283
lbl250:
                    // 1 sources

                    var18_44 = -i - 1;
                    var20_24 = 0;
                    for (var14_20 = 1; var14_20 < j; ++var14_20) {
                        var11_46 = var18_44 += i;
                        var21_25 = var20_24 += var19_12;
                        for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                            var11_46 += 2;
                            var22_26 = var21_25 += 2;
                            for (var15_21 = 0; var15_21 < k; ++var15_21) {
                                gs[var22_26 - 1] = ks[m + var11_46 - 1] * is[var22_26 - 1] - ks[m + var11_46] * is[var22_26];
                                gs[var22_26] = ks[m + var11_46 - 1] * is[var22_26] + ks[m + var11_46] * is[var22_26 - 1];
                                var22_26 += i;
                            }
                        }
                    }
                    return;
                }
                case 139: {
                    var18_45 = -i - 1;
                    var20_24 = 0;
                    for (var14_20 = 1; var14_20 < j; ++var14_20) {
                        var18_45 += i;
                        var21_25 = var20_24 += var19_12;
                        for (var15_21 = 0; var15_21 < k; ++var15_21) {
                            var11_47 = var18_45;
                            var22_26 = var21_25;
                            for (var13_19 = 2; var13_19 < i; var13_19 += 2) {
                                gs[(var22_26 += 2) - 1] = ks[m + (var11_47 += 2) - 1] * is[var22_26 - 1] - ks[m + var11_47] * is[var22_26];
                                gs[var22_26] = ks[m + var11_47 - 1] * is[var22_26] + ks[m + var11_47] * is[var22_26 - 1];
                            }
                            var21_25 += i;
                        }
                    }
                    break block10;
                }
lbl283:
                // 10 sources

                default: {
                    continue block10;
                }
            }
            break;
        }
    }

    /*
     * Enabled aggressive block sorting
     */
    static void drftb1(int i, float[] fs, float[] gs, float[] hs, int j, int[] is) {
        int n = 0;
        int n2 = 0;
        int n3 = 0;
        int n4 = 0;
        int n5 = is[1];
        int n6 = 0;
        int n7 = 1;
        int n8 = 1;
        int n9 = 0;
        while (true) {
            if (n9 >= n5) {
                if (n6 == 0) {
                    return;
                }
                int n10 = 0;
                while (true) {
                    if (n10 >= i) {
                        return;
                    }
                    fs[n10] = gs[n10];
                    ++n10;
                }
            }
            int n11 = 100;
            block9: while (true) {
                switch (n11) {
                    case 100: {
                        n2 = is[n9 + 2];
                        n = n2 * n7;
                        n3 = i / n;
                        n4 = n3 * n7;
                        if (n2 != 4) {
                            n11 = 103;
                            break;
                        }
                        int n12 = n8 + n3;
                        int n13 = n12 + n3;
                        if (n6 != 0) {
                            Drft.dradb4(n3, n7, gs, fs, hs, j + n8 - 1, hs, j + n12 - 1, hs, j + n13 - 1);
                        } else {
                            Drft.dradb4(n3, n7, fs, gs, hs, j + n8 - 1, hs, j + n12 - 1, hs, j + n13 - 1);
                        }
                        n6 = 1 - n6;
                        n11 = 115;
                        break;
                    }
                    case 103: {
                        if (n2 != 2) {
                            n11 = 106;
                            break;
                        }
                        if (n6 != 0) {
                            Drft.dradb2(n3, n7, gs, fs, hs, j + n8 - 1);
                        } else {
                            Drft.dradb2(n3, n7, fs, gs, hs, j + n8 - 1);
                        }
                        n6 = 1 - n6;
                        n11 = 115;
                        break;
                    }
                    case 106: {
                        if (n2 != 3) {
                            n11 = 109;
                            break;
                        }
                        int n12 = n8 + n3;
                        if (n6 != 0) {
                            Drft.dradb3(n3, n7, gs, fs, hs, j + n8 - 1, hs, j + n12 - 1);
                        } else {
                            Drft.dradb3(n3, n7, fs, gs, hs, j + n8 - 1, hs, j + n12 - 1);
                        }
                        n6 = 1 - n6;
                        n11 = 115;
                        break;
                    }
                    case 109: {
                        if (n6 != 0) {
                            Drft.dradbg(n3, n2, n7, n4, gs, gs, gs, fs, fs, hs, j + n8 - 1);
                        } else {
                            Drft.dradbg(n3, n2, n7, n4, fs, fs, fs, gs, gs, hs, j + n8 - 1);
                        }
                        if (n3 == 1) {
                            n6 = 1 - n6;
                        }
                    }
                    case 115: {
                        n7 = n;
                        n8 += (n2 - 1) * n3;
                        break block9;
                    }
                }
            }
            ++n9;
        }
    }
}

