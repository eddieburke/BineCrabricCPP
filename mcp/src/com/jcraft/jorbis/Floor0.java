/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jogg.Buffer;
import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.CodeBook;
import com.jcraft.jorbis.DspState;
import com.jcraft.jorbis.FuncFloor;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.InfoMode;
import com.jcraft.jorbis.Lpc;
import com.jcraft.jorbis.Lsp;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
class Floor0
extends FuncFloor {
    float[] lsp = null;

    Floor0() {
    }

    void pack(Object object, Buffer buffer) {
        InfoFloor0 infoFloor0 = (InfoFloor0)object;
        buffer.write(infoFloor0.order, 8);
        buffer.write(infoFloor0.rate, 16);
        buffer.write(infoFloor0.barkmap, 16);
        buffer.write(infoFloor0.ampbits, 6);
        buffer.write(infoFloor0.ampdB, 8);
        buffer.write(infoFloor0.numbooks - 1, 4);
        for (int i = 0; i < infoFloor0.numbooks; ++i) {
            buffer.write(infoFloor0.books[i], 8);
        }
    }

    Object unpack(Info info, Buffer buffer) {
        InfoFloor0 infoFloor0 = new InfoFloor0();
        infoFloor0.order = buffer.read(8);
        infoFloor0.rate = buffer.read(16);
        infoFloor0.barkmap = buffer.read(16);
        infoFloor0.ampbits = buffer.read(6);
        infoFloor0.ampdB = buffer.read(8);
        infoFloor0.numbooks = buffer.read(4) + 1;
        if (infoFloor0.order < 1 || infoFloor0.rate < 1 || infoFloor0.barkmap < 1 || infoFloor0.numbooks < 1) {
            return null;
        }
        for (int i = 0; i < infoFloor0.numbooks; ++i) {
            infoFloor0.books[i] = buffer.read(8);
            if (infoFloor0.books[i] >= 0 && infoFloor0.books[i] < info.books) continue;
            return null;
        }
        return infoFloor0;
    }

    Object look(DspState dspState, InfoMode infoMode, Object object) {
        Info info = dspState.vi;
        InfoFloor0 infoFloor0 = (InfoFloor0)object;
        LookFloor0 lookFloor0 = new LookFloor0();
        lookFloor0.m = infoFloor0.order;
        lookFloor0.n = info.blocksizes[infoMode.blockflag] / 2;
        lookFloor0.ln = infoFloor0.barkmap;
        lookFloor0.vi = infoFloor0;
        lookFloor0.lpclook.init(lookFloor0.ln, lookFloor0.m);
        float f = (float)lookFloor0.ln / Floor0.toBARK((float)((double)infoFloor0.rate / 2.0));
        lookFloor0.linearmap = new int[lookFloor0.n];
        for (int i = 0; i < lookFloor0.n; ++i) {
            int n = MathHelper.floor(Floor0.toBARK((float)((double)infoFloor0.rate / 2.0 / (double)lookFloor0.n * (double)i)) * f);
            if (n >= lookFloor0.ln) {
                n = lookFloor0.ln;
            }
            lookFloor0.linearmap[i] = n;
        }
        return lookFloor0;
    }

    static float toBARK(float f) {
        return (float)(13.1 * Math.atan(7.4E-4 * (double)f) + 2.24 * Math.atan((double)(f * f) * 1.85E-8) + 1.0E-4 * (double)f);
    }

    Object state(Object object) {
        EchstateFloor0 echstateFloor0 = new EchstateFloor0();
        InfoFloor0 infoFloor0 = (InfoFloor0)object;
        echstateFloor0.codewords = new int[infoFloor0.order];
        echstateFloor0.curve = new float[infoFloor0.barkmap];
        echstateFloor0.frameno = -1L;
        return echstateFloor0;
    }

    void free_info(Object object) {
    }

    void free_look(Object object) {
    }

    void free_state(Object object) {
    }

    int forward(Block block, Object object, float[] fs, float[] gs, Object object2) {
        return 0;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    int inverse(Block block, Object object, float[] fs) {
        LookFloor0 lookFloor0 = (LookFloor0)object;
        InfoFloor0 infoFloor0 = lookFloor0.vi;
        int n = block.opb.read(infoFloor0.ampbits);
        if (n > 0) {
            int n2 = (1 << infoFloor0.ampbits) - 1;
            float f = (float)n / (float)n2 * (float)infoFloor0.ampdB;
            int n3 = block.opb.read(Util.ilog(infoFloor0.numbooks));
            if (n3 != -1 && n3 < infoFloor0.numbooks) {
                Floor0 floor0 = this;
                synchronized (floor0) {
                    int n4;
                    if (this.lsp == null || this.lsp.length < lookFloor0.m) {
                        this.lsp = new float[lookFloor0.m];
                    } else {
                        for (int i = 0; i < lookFloor0.m; ++i) {
                            this.lsp[i] = 0.0f;
                        }
                    }
                    CodeBook codeBook = block.vd.fullbooks[infoFloor0.books[n3]];
                    float f2 = 0.0f;
                    for (n4 = 0; n4 < lookFloor0.m; ++n4) {
                        fs[n4] = 0.0f;
                    }
                    for (n4 = 0; n4 < lookFloor0.m; n4 += codeBook.dim) {
                        if (codeBook.decodevs(this.lsp, n4, block.opb, 1, -1) != -1) continue;
                        for (int i = 0; i < lookFloor0.n; ++i) {
                            fs[i] = 0.0f;
                        }
                        return 0;
                    }
                    n4 = 0;
                    while (n4 < lookFloor0.m) {
                        for (int i = 0; i < codeBook.dim; ++i) {
                            int n5 = n4++;
                            this.lsp[n5] = this.lsp[n5] + f2;
                        }
                        f2 = this.lsp[n4 - 1];
                    }
                    Lsp.lsp_to_curve(fs, lookFloor0.linearmap, lookFloor0.n, lookFloor0.ln, this.lsp, lookFloor0.m, f, infoFloor0.ampdB);
                    return 1;
                }
            }
        }
        return 0;
    }

    Object inverse1(Block block, Object object, Object object2) {
        int n;
        LookFloor0 lookFloor0 = (LookFloor0)object;
        InfoFloor0 infoFloor0 = lookFloor0.vi;
        float[] fArray = null;
        if (object2 instanceof float[]) {
            fArray = (float[])object2;
        }
        if ((n = block.opb.read(infoFloor0.ampbits)) > 0) {
            int n2 = (1 << infoFloor0.ampbits) - 1;
            float f = (float)n / (float)n2 * (float)infoFloor0.ampdB;
            int n3 = block.opb.read(Util.ilog(infoFloor0.numbooks));
            if (n3 != -1 && n3 < infoFloor0.numbooks) {
                int n4;
                CodeBook codeBook = block.vd.fullbooks[infoFloor0.books[n3]];
                float f2 = 0.0f;
                if (fArray == null || fArray.length < lookFloor0.m + 1) {
                    fArray = new float[lookFloor0.m + 1];
                } else {
                    for (n4 = 0; n4 < fArray.length; ++n4) {
                        fArray[n4] = 0.0f;
                    }
                }
                for (n4 = 0; n4 < lookFloor0.m; n4 += codeBook.dim) {
                    if (codeBook.decodev_set(fArray, n4, block.opb, codeBook.dim) != -1) continue;
                    return null;
                }
                n4 = 0;
                while (n4 < lookFloor0.m) {
                    for (int i = 0; i < codeBook.dim; ++i) {
                        int n5 = n4++;
                        fArray[n5] = fArray[n5] + f2;
                    }
                    f2 = fArray[n4 - 1];
                }
                fArray[lookFloor0.m] = f;
                return fArray;
            }
        }
        return null;
    }

    int inverse2(Block block, Object object, Object object2, float[] fs) {
        LookFloor0 lookFloor0 = (LookFloor0)object;
        InfoFloor0 infoFloor0 = lookFloor0.vi;
        if (object2 != null) {
            float[] fArray = (float[])object2;
            float f = fArray[lookFloor0.m];
            Lsp.lsp_to_curve(fs, lookFloor0.linearmap, lookFloor0.n, lookFloor0.ln, fArray, lookFloor0.m, f, infoFloor0.ampdB);
            return 1;
        }
        for (int i = 0; i < lookFloor0.n; ++i) {
            fs[i] = 0.0f;
        }
        return 0;
    }

    static float fromdB(float f) {
        return (float)Math.exp((double)f * 0.11512925);
    }

    static void lsp_to_lpc(float[] fs, float[] gs, int i) {
        int n;
        int n2;
        int n3 = i / 2;
        float[] fArray = new float[n3];
        float[] fArray2 = new float[n3];
        float[] fArray3 = new float[n3 + 1];
        float[] fArray4 = new float[n3 + 1];
        float[] fArray5 = new float[n3];
        float[] fArray6 = new float[n3];
        for (n2 = 0; n2 < n3; ++n2) {
            fArray[n2] = (float)(-2.0 * Math.cos(fs[n2 * 2]));
            fArray2[n2] = (float)(-2.0 * Math.cos(fs[n2 * 2 + 1]));
        }
        for (n = 0; n < n3; ++n) {
            fArray3[n] = 0.0f;
            fArray4[n] = 1.0f;
            fArray5[n] = 0.0f;
            fArray6[n] = 1.0f;
        }
        fArray4[n] = 1.0f;
        fArray3[n] = 1.0f;
        for (n2 = 1; n2 < i + 1; ++n2) {
            float f = 0.0f;
            float f2 = 0.0f;
            for (n = 0; n < n3; ++n) {
                float f3 = fArray[n] * fArray4[n] + fArray3[n];
                fArray3[n] = fArray4[n];
                fArray4[n] = f2;
                f2 += f3;
                f3 = fArray2[n] * fArray6[n] + fArray5[n];
                fArray5[n] = fArray6[n];
                fArray6[n] = f;
                f += f3;
            }
            gs[n2 - 1] = (f2 + fArray4[n] + f - fArray3[n]) / 2.0f;
            fArray4[n] = f2;
            fArray3[n] = f;
        }
    }

    static void lpc_to_curve(float[] fs, float[] gs, float f, LookFloor0 lookFloor0, String string, int i) {
        float[] fArray = new float[Math.max(lookFloor0.ln * 2, lookFloor0.m * 2 + 2)];
        if (f == 0.0f) {
            for (int j = 0; j < lookFloor0.n; ++j) {
                fs[j] = 0.0f;
            }
            return;
        }
        lookFloor0.lpclook.lpc_to_curve(fArray, gs, f);
        for (int j = 0; j < lookFloor0.n; ++j) {
            fs[j] = fArray[lookFloor0.linearmap[j]];
        }
    }

    @Environment(value=EnvType.CLIENT)
    class EchstateFloor0 {
        int[] codewords;
        float[] curve;
        long frameno;
        long codes;

        EchstateFloor0() {
        }
    }

    @Environment(value=EnvType.CLIENT)
    class InfoFloor0 {
        int order;
        int rate;
        int barkmap;
        int ampbits;
        int ampdB;
        int numbooks;
        int[] books = new int[16];

        InfoFloor0() {
        }
    }

    @Environment(value=EnvType.CLIENT)
    class LookFloor0 {
        int n;
        int ln;
        int m;
        int[] linearmap;
        InfoFloor0 vi;
        Lpc lpclook = new Lpc();

        LookFloor0() {
        }
    }
}

