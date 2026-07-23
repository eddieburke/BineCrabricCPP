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
import com.jcraft.jorbis.FuncResidue;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.InfoMode;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Residue0
extends FuncResidue {
    private static int[][][] _01inverse_partword = new int[2][][];
    static int[][] _2inverse_partword = null;

    Residue0() {
    }

    void pack(Object object, Buffer buffer) {
        int n;
        InfoResidue0 infoResidue0 = (InfoResidue0)object;
        int n2 = 0;
        buffer.write(infoResidue0.begin, 24);
        buffer.write(infoResidue0.end, 24);
        buffer.write(infoResidue0.grouping - 1, 24);
        buffer.write(infoResidue0.partitions - 1, 6);
        buffer.write(infoResidue0.groupbook, 8);
        for (n = 0; n < infoResidue0.partitions; ++n) {
            int n3 = infoResidue0.secondstages[n];
            if (Util.ilog(n3) > 3) {
                buffer.write(n3, 3);
                buffer.write(1, 1);
                buffer.write(n3 >>> 3, 5);
            } else {
                buffer.write(n3, 4);
            }
            n2 += Util.icount(n3);
        }
        for (n = 0; n < n2; ++n) {
            buffer.write(infoResidue0.booklist[n], 8);
        }
    }

    Object unpack(Info info, Buffer buffer) {
        int n;
        int n2 = 0;
        InfoResidue0 infoResidue0 = new InfoResidue0();
        infoResidue0.begin = buffer.read(24);
        infoResidue0.end = buffer.read(24);
        infoResidue0.grouping = buffer.read(24) + 1;
        infoResidue0.partitions = buffer.read(6) + 1;
        infoResidue0.groupbook = buffer.read(8);
        for (n = 0; n < infoResidue0.partitions; ++n) {
            int n3 = buffer.read(3);
            if (buffer.read(1) != 0) {
                n3 |= buffer.read(5) << 3;
            }
            infoResidue0.secondstages[n] = n3;
            n2 += Util.icount(n3);
        }
        for (n = 0; n < n2; ++n) {
            infoResidue0.booklist[n] = buffer.read(8);
        }
        if (infoResidue0.groupbook >= info.books) {
            this.free_info(infoResidue0);
            return null;
        }
        for (n = 0; n < n2; ++n) {
            if (infoResidue0.booklist[n] < info.books) continue;
            this.free_info(infoResidue0);
            return null;
        }
        return infoResidue0;
    }

    Object look(DspState dspState, InfoMode infoMode, Object object) {
        int n;
        int n2;
        int n3;
        int n4;
        InfoResidue0 infoResidue0 = (InfoResidue0)object;
        LookResidue0 lookResidue0 = new LookResidue0();
        int n5 = 0;
        int n6 = 0;
        lookResidue0.info = infoResidue0;
        lookResidue0.map = infoMode.mapping;
        lookResidue0.parts = infoResidue0.partitions;
        lookResidue0.fullbooks = dspState.fullbooks;
        lookResidue0.phrasebook = dspState.fullbooks[infoResidue0.groupbook];
        int n7 = lookResidue0.phrasebook.dim;
        lookResidue0.partbooks = new int[lookResidue0.parts][];
        for (n4 = 0; n4 < lookResidue0.parts; ++n4) {
            n3 = infoResidue0.secondstages[n4];
            n2 = Util.ilog(n3);
            if (n2 == 0) continue;
            if (n2 > n6) {
                n6 = n2;
            }
            lookResidue0.partbooks[n4] = new int[n2];
            for (n = 0; n < n2; ++n) {
                if ((n3 & 1 << n) == 0) continue;
                lookResidue0.partbooks[n4][n] = infoResidue0.booklist[n5++];
            }
        }
        lookResidue0.partvals = (int)Math.rint(Math.pow(lookResidue0.parts, n7));
        lookResidue0.stages = n6;
        lookResidue0.decodemap = new int[lookResidue0.partvals][];
        for (n4 = 0; n4 < lookResidue0.partvals; ++n4) {
            n3 = n4;
            n2 = lookResidue0.partvals / lookResidue0.parts;
            lookResidue0.decodemap[n4] = new int[n7];
            for (n = 0; n < n7; ++n) {
                int n8 = n3 / n2;
                n3 -= n8 * n2;
                n2 /= lookResidue0.parts;
                lookResidue0.decodemap[n4][n] = n8;
            }
        }
        return lookResidue0;
    }

    void free_info(Object object) {
    }

    void free_look(Object object) {
    }

    static synchronized int _01inverse(Block block, Object object, float[][] fs, int i, int j) {
        int n;
        LookResidue0 lookResidue0 = (LookResidue0)object;
        InfoResidue0 infoResidue0 = lookResidue0.info;
        int n2 = infoResidue0.grouping;
        int n3 = lookResidue0.phrasebook.dim;
        int n4 = infoResidue0.end - infoResidue0.begin;
        int n5 = n4 / n2;
        int n6 = (n5 + n3 - 1) / n3;
        if (_01inverse_partword.length < i) {
            _01inverse_partword = new int[i][][];
        }
        for (n = 0; n < i; ++n) {
            if (_01inverse_partword[n] != null && _01inverse_partword[n].length >= n6) continue;
            Residue0._01inverse_partword[n] = new int[n6][];
        }
        for (int k = 0; k < lookResidue0.stages; ++k) {
            int n7 = 0;
            int n8 = 0;
            while (n7 < n5) {
                int n9;
                if (k == 0) {
                    for (n = 0; n < i; ++n) {
                        n9 = lookResidue0.phrasebook.decode(block.opb);
                        if (n9 == -1) {
                            return 0;
                        }
                        Residue0._01inverse_partword[n][n8] = lookResidue0.decodemap[n9];
                        if (_01inverse_partword[n][n8] != null) continue;
                        return 0;
                    }
                }
                for (int i2 = 0; i2 < n3 && n7 < n5; ++i2, ++n7) {
                    for (n = 0; n < i; ++n) {
                        CodeBook codeBook;
                        n9 = infoResidue0.begin + n7 * n2;
                        int n10 = _01inverse_partword[n][n8][i2];
                        if ((infoResidue0.secondstages[n10] & 1 << k) == 0 || (codeBook = lookResidue0.fullbooks[lookResidue0.partbooks[n10][k]]) == null || !(j == 0 ? codeBook.decodevs_add(fs[n], n9, block.opb, n2) == -1 : j == 1 && codeBook.decodev_add(fs[n], n9, block.opb, n2) == -1)) continue;
                        return 0;
                    }
                }
                ++n8;
            }
        }
        return 0;
    }

    static synchronized int _2inverse(Block block, Object object, float[][] fs, int i) {
        LookResidue0 lookResidue0 = (LookResidue0)object;
        InfoResidue0 infoResidue0 = lookResidue0.info;
        int n = infoResidue0.grouping;
        int n2 = lookResidue0.phrasebook.dim;
        int n3 = infoResidue0.end - infoResidue0.begin;
        int n4 = n3 / n;
        int n5 = (n4 + n2 - 1) / n2;
        if (_2inverse_partword == null || _2inverse_partword.length < n5) {
            _2inverse_partword = new int[n5][];
        }
        for (int j = 0; j < lookResidue0.stages; ++j) {
            int n6 = 0;
            int n7 = 0;
            while (n6 < n4) {
                int n8;
                if (j == 0) {
                    n8 = lookResidue0.phrasebook.decode(block.opb);
                    if (n8 == -1) {
                        return 0;
                    }
                    Residue0._2inverse_partword[n7] = lookResidue0.decodemap[n8];
                    if (_2inverse_partword[n7] == null) {
                        return 0;
                    }
                }
                for (int k = 0; k < n2 && n6 < n4; ++k, ++n6) {
                    CodeBook codeBook;
                    n8 = infoResidue0.begin + n6 * n;
                    int n9 = _2inverse_partword[n7][k];
                    if ((infoResidue0.secondstages[n9] & 1 << j) == 0 || (codeBook = lookResidue0.fullbooks[lookResidue0.partbooks[n9][j]]) == null || codeBook.decodevv_add(fs, n8, i, block.opb, n) != -1) continue;
                    return 0;
                }
                ++n7;
            }
        }
        return 0;
    }

    int inverse(Block block, Object object, float[][] fs, int[] is, int i) {
        int n = 0;
        for (int j = 0; j < i; ++j) {
            if (is[j] == 0) continue;
            fs[n++] = fs[j];
        }
        if (n != 0) {
            return Residue0._01inverse(block, object, fs, n, 0);
        }
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    class InfoResidue0 {
        int begin;
        int end;
        int grouping;
        int partitions;
        int groupbook;
        int[] secondstages = new int[64];
        int[] booklist = new int[256];
        float[] entmax = new float[64];
        float[] ampmax = new float[64];
        int[] subgrp = new int[64];
        int[] blimit = new int[64];

        InfoResidue0() {
        }
    }

    @Environment(value=EnvType.CLIENT)
    class LookResidue0 {
        InfoResidue0 info;
        int map;
        int parts;
        int stages;
        CodeBook[] fullbooks;
        CodeBook phrasebook;
        int[][] partbooks;
        int partvals;
        int[][] decodemap;
        int postbits;
        int phrasebits;
        int frames;

        LookResidue0() {
        }
    }
}

