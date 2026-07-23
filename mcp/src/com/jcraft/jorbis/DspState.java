/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.CodeBook;
import com.jcraft.jorbis.FuncMapping;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.Mdct;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class DspState {
    static final float M_PI = (float)Math.PI;
    static final int VI_TRANSFORMB = 1;
    static final int VI_WINDOWB = 1;
    int analysisp;
    Info vi;
    int modebits;
    float[][] pcm;
    int pcm_storage;
    int pcm_current;
    int pcm_returned;
    float[] multipliers;
    int envelope_storage;
    int envelope_current;
    int eofflag;
    int lW;
    int W;
    int nW;
    int centerW;
    long granulepos;
    long sequence;
    long glue_bits;
    long time_bits;
    long floor_bits;
    long res_bits;
    float[][][][][] window;
    Object[][] transform = new Object[2][];
    CodeBook[] fullbooks;
    Object[] mode;
    byte[] header;
    byte[] header1;
    byte[] header2;

    public DspState() {
        this.window = new float[2][][][][];
        this.window[0] = new float[2][][][];
        this.window[0][0] = new float[2][][];
        this.window[0][1] = new float[2][][];
        this.window[0][0][0] = new float[2][];
        this.window[0][0][1] = new float[2][];
        this.window[0][1][0] = new float[2][];
        this.window[0][1][1] = new float[2][];
        this.window[1] = new float[2][][][];
        this.window[1][0] = new float[2][][];
        this.window[1][1] = new float[2][][];
        this.window[1][0][0] = new float[2][];
        this.window[1][0][1] = new float[2][];
        this.window[1][1][0] = new float[2][];
        this.window[1][1][1] = new float[2][];
    }

    static float[] window(int i, int j, int k, int l) {
        float[] fArray = new float[j];
        switch (i) {
            case 0: {
                float f;
                int n;
                int n2 = j / 4 - k / 2;
                int n3 = j - j / 4 - l / 2;
                for (n = 0; n < k; ++n) {
                    f = (float)(((double)n + 0.5) / (double)k * 3.1415927410125732 / 2.0);
                    f = (float)Math.sin(f);
                    f *= f;
                    f = (float)((double)f * 1.5707963705062866);
                    fArray[n + n2] = f = (float)Math.sin(f);
                }
                for (n = n2 + k; n < n3; ++n) {
                    fArray[n] = 1.0f;
                }
                for (n = 0; n < l; ++n) {
                    f = (float)(((double)(l - n) - 0.5) / (double)l * 3.1415927410125732 / 2.0);
                    f = (float)Math.sin(f);
                    f *= f;
                    f = (float)((double)f * 1.5707963705062866);
                    fArray[n + n3] = f = (float)Math.sin(f);
                }
                break;
            }
            default: {
                return null;
            }
        }
        return fArray;
    }

    int init(Info info, boolean bl) {
        int n;
        this.vi = info;
        this.modebits = Util.ilog2(info.modes);
        this.transform[0] = new Object[1];
        this.transform[1] = new Object[1];
        this.transform[0][0] = new Mdct();
        this.transform[1][0] = new Mdct();
        ((Mdct)this.transform[0][0]).init(info.blocksizes[0]);
        ((Mdct)this.transform[1][0]).init(info.blocksizes[1]);
        this.window[0][0][0] = new float[1][];
        this.window[0][0][1] = this.window[0][0][0];
        this.window[0][1][0] = this.window[0][0][0];
        this.window[0][1][1] = this.window[0][0][0];
        this.window[1][0][0] = new float[1][];
        this.window[1][0][1] = new float[1][];
        this.window[1][1][0] = new float[1][];
        this.window[1][1][1] = new float[1][];
        for (n = 0; n < 1; ++n) {
            this.window[0][0][0][n] = DspState.window(n, info.blocksizes[0], info.blocksizes[0] / 2, info.blocksizes[0] / 2);
            this.window[1][0][0][n] = DspState.window(n, info.blocksizes[1], info.blocksizes[0] / 2, info.blocksizes[0] / 2);
            this.window[1][0][1][n] = DspState.window(n, info.blocksizes[1], info.blocksizes[0] / 2, info.blocksizes[1] / 2);
            this.window[1][1][0][n] = DspState.window(n, info.blocksizes[1], info.blocksizes[1] / 2, info.blocksizes[0] / 2);
            this.window[1][1][1][n] = DspState.window(n, info.blocksizes[1], info.blocksizes[1] / 2, info.blocksizes[1] / 2);
        }
        this.fullbooks = new CodeBook[info.books];
        for (n = 0; n < info.books; ++n) {
            this.fullbooks[n] = new CodeBook();
            this.fullbooks[n].init_decode(info.book_param[n]);
        }
        this.pcm_storage = 8192;
        this.pcm = new float[info.channels][];
        for (n = 0; n < info.channels; ++n) {
            this.pcm[n] = new float[this.pcm_storage];
        }
        this.lW = 0;
        this.W = 0;
        this.pcm_current = this.centerW = info.blocksizes[1] / 2;
        this.mode = new Object[info.modes];
        for (n = 0; n < info.modes; ++n) {
            int n2 = info.mode_param[n].mapping;
            int n3 = info.map_type[n2];
            this.mode[n] = FuncMapping.mapping_P[n3].look(this, info.mode_param[n], info.map_param[n2]);
        }
        return 0;
    }

    public int synthesis_init(Info info) {
        this.init(info, false);
        this.pcm_returned = this.centerW;
        this.centerW -= info.blocksizes[this.W] / 4 + info.blocksizes[this.lW] / 4;
        this.granulepos = -1L;
        this.sequence = -1L;
        return 0;
    }

    DspState(Info info) {
        this();
        this.init(info, false);
        this.pcm_returned = this.centerW;
        this.centerW -= info.blocksizes[this.W] / 4 + info.blocksizes[this.lW] / 4;
        this.granulepos = -1L;
        this.sequence = -1L;
    }

    public int synthesis_blockin(Block block) {
        int n;
        int n2;
        int n3;
        if (this.centerW > this.vi.blocksizes[1] / 2 && this.pcm_returned > 8192) {
            n3 = this.centerW - this.vi.blocksizes[1] / 2;
            n3 = this.pcm_returned < n3 ? this.pcm_returned : n3;
            this.pcm_current -= n3;
            this.centerW -= n3;
            this.pcm_returned -= n3;
            if (n3 != 0) {
                for (n2 = 0; n2 < this.vi.channels; ++n2) {
                    System.arraycopy(this.pcm[n2], n3, this.pcm[n2], 0, this.pcm_current);
                }
            }
        }
        this.lW = this.W;
        this.W = block.W;
        this.nW = -1;
        this.glue_bits += (long)block.glue_bits;
        this.time_bits += (long)block.time_bits;
        this.floor_bits += (long)block.floor_bits;
        this.res_bits += (long)block.res_bits;
        if (this.sequence + 1L != block.sequence) {
            this.granulepos = -1L;
        }
        this.sequence = block.sequence;
        n3 = this.vi.blocksizes[this.W];
        n2 = this.centerW + this.vi.blocksizes[this.lW] / 4 + n3 / 4;
        int n4 = n2 - n3 / 2;
        int n5 = n4 + n3;
        int n6 = 0;
        int n7 = 0;
        if (n5 > this.pcm_storage) {
            this.pcm_storage = n5 + this.vi.blocksizes[1];
            for (n = 0; n < this.vi.channels; ++n) {
                float[] fArray = new float[this.pcm_storage];
                System.arraycopy(this.pcm[n], 0, fArray, 0, this.pcm[n].length);
                this.pcm[n] = fArray;
            }
        }
        switch (this.W) {
            case 0: {
                n6 = 0;
                n7 = this.vi.blocksizes[0] / 2;
                break;
            }
            case 1: {
                n6 = this.vi.blocksizes[1] / 4 - this.vi.blocksizes[this.lW] / 4;
                n7 = n6 + this.vi.blocksizes[this.lW] / 2;
            }
        }
        for (n = 0; n < this.vi.channels; ++n) {
            int n8 = n4;
            int n9 = 0;
            for (n9 = n6; n9 < n7; ++n9) {
                float[] fArray = this.pcm[n];
                int n10 = n8 + n9;
                fArray[n10] = fArray[n10] + block.pcm[n][n9];
            }
            while (n9 < n3) {
                this.pcm[n][n8 + n9] = block.pcm[n][n9];
                ++n9;
            }
        }
        if (this.granulepos == -1L) {
            this.granulepos = block.granulepos;
        } else {
            this.granulepos += (long)(n2 - this.centerW);
            if (block.granulepos != -1L && this.granulepos != block.granulepos) {
                if (this.granulepos > block.granulepos && block.eofflag != 0) {
                    n2 = (int)((long)n2 - (this.granulepos - block.granulepos));
                }
                this.granulepos = block.granulepos;
            }
        }
        this.centerW = n2;
        this.pcm_current = n5;
        if (block.eofflag != 0) {
            this.eofflag = 1;
        }
        return 0;
    }

    public int synthesis_pcmout(float[][][] fs, int[] is) {
        if (this.pcm_returned < this.centerW) {
            if (fs != null) {
                for (int i = 0; i < this.vi.channels; ++i) {
                    is[i] = this.pcm_returned;
                }
                fs[0] = this.pcm;
            }
            return this.centerW - this.pcm_returned;
        }
        return 0;
    }

    public int synthesis_read(int i) {
        if (i != 0 && this.pcm_returned + i > this.centerW) {
            return -1;
        }
        this.pcm_returned += i;
        return 0;
    }

    public void clear() {
    }
}

