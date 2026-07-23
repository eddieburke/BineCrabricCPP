/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jogg.Buffer;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
class StaticCodeBook {
    int dim;
    int entries;
    int[] lengthlist;
    int maptype;
    int q_min;
    int q_delta;
    int q_quant;
    int q_sequencep;
    int[] quantlist;
    static final int VQ_FEXP = 10;
    static final int VQ_FMAN = 21;
    static final int VQ_FEXP_BIAS = 768;

    StaticCodeBook() {
    }

    int pack(Buffer buffer) {
        int n;
        int n2;
        boolean bl = false;
        buffer.write(5653314, 24);
        buffer.write(this.dim, 16);
        buffer.write(this.entries, 24);
        for (n2 = 1; n2 < this.entries && this.lengthlist[n2] >= this.lengthlist[n2 - 1]; ++n2) {
        }
        if (n2 == this.entries) {
            bl = true;
        }
        if (bl) {
            n = 0;
            buffer.write(1, 1);
            buffer.write(this.lengthlist[0] - 1, 5);
            for (n2 = 1; n2 < this.entries; ++n2) {
                int n3 = this.lengthlist[n2];
                int n4 = this.lengthlist[n2 - 1];
                if (n3 <= n4) continue;
                for (int i = n4; i < n3; ++i) {
                    buffer.write(n2 - n, Util.ilog(this.entries - n));
                    n = n2;
                }
            }
            buffer.write(n2 - n, Util.ilog(this.entries - n));
        } else {
            buffer.write(0, 1);
            for (n2 = 0; n2 < this.entries && this.lengthlist[n2] != 0; ++n2) {
            }
            if (n2 == this.entries) {
                buffer.write(0, 1);
                for (n2 = 0; n2 < this.entries; ++n2) {
                    buffer.write(this.lengthlist[n2] - 1, 5);
                }
            } else {
                buffer.write(1, 1);
                for (n2 = 0; n2 < this.entries; ++n2) {
                    if (this.lengthlist[n2] == 0) {
                        buffer.write(0, 1);
                        continue;
                    }
                    buffer.write(1, 1);
                    buffer.write(this.lengthlist[n2] - 1, 5);
                }
            }
        }
        buffer.write(this.maptype, 4);
        switch (this.maptype) {
            case 0: {
                break;
            }
            case 1: 
            case 2: {
                if (this.quantlist == null) {
                    return -1;
                }
                buffer.write(this.q_min, 32);
                buffer.write(this.q_delta, 32);
                buffer.write(this.q_quant - 1, 4);
                buffer.write(this.q_sequencep, 1);
                n = 0;
                switch (this.maptype) {
                    case 1: {
                        n = this.maptype1_quantvals();
                        break;
                    }
                    case 2: {
                        n = this.entries * this.dim;
                    }
                }
                for (n2 = 0; n2 < n; ++n2) {
                    buffer.write(Math.abs(this.quantlist[n2]), this.q_quant);
                }
                break;
            }
            default: {
                return -1;
            }
        }
        return 0;
    }

    int unpack(Buffer buffer) {
        int n;
        int n2;
        if (buffer.read(24) != 5653314) {
            this.clear();
            return -1;
        }
        this.dim = buffer.read(16);
        this.entries = buffer.read(24);
        if (this.entries == -1) {
            this.clear();
            return -1;
        }
        switch (buffer.read(1)) {
            case 0: {
                this.lengthlist = new int[this.entries];
                if (buffer.read(1) != 0) {
                    for (n2 = 0; n2 < this.entries; ++n2) {
                        if (buffer.read(1) != 0) {
                            n = buffer.read(5);
                            if (n == -1) {
                                this.clear();
                                return -1;
                            }
                            this.lengthlist[n2] = n + 1;
                            continue;
                        }
                        this.lengthlist[n2] = 0;
                    }
                } else {
                    for (n2 = 0; n2 < this.entries; ++n2) {
                        n = buffer.read(5);
                        if (n == -1) {
                            this.clear();
                            return -1;
                        }
                        this.lengthlist[n2] = n + 1;
                    }
                }
                break;
            }
            case 1: {
                n = buffer.read(5) + 1;
                this.lengthlist = new int[this.entries];
                n2 = 0;
                while (n2 < this.entries) {
                    int n3 = buffer.read(Util.ilog(this.entries - n2));
                    if (n3 == -1) {
                        this.clear();
                        return -1;
                    }
                    int n4 = 0;
                    while (n4 < n3) {
                        this.lengthlist[n2] = n;
                        ++n4;
                        ++n2;
                    }
                    ++n;
                }
                break;
            }
            default: {
                return -1;
            }
        }
        this.maptype = buffer.read(4);
        switch (this.maptype) {
            case 0: {
                break;
            }
            case 1: 
            case 2: {
                this.q_min = buffer.read(32);
                this.q_delta = buffer.read(32);
                this.q_quant = buffer.read(4) + 1;
                this.q_sequencep = buffer.read(1);
                n = 0;
                switch (this.maptype) {
                    case 1: {
                        n = this.maptype1_quantvals();
                        break;
                    }
                    case 2: {
                        n = this.entries * this.dim;
                    }
                }
                this.quantlist = new int[n];
                for (n2 = 0; n2 < n; ++n2) {
                    this.quantlist[n2] = buffer.read(this.q_quant);
                }
                if (this.quantlist[n - 1] != -1) break;
                this.clear();
                return -1;
            }
            default: {
                this.clear();
                return -1;
            }
        }
        return 0;
    }

    private int maptype1_quantvals() {
        int n = MathHelper.floor(Math.pow(this.entries, 1.0 / (double)this.dim));
        while (true) {
            int n2 = 1;
            int n3 = 1;
            for (int i = 0; i < this.dim; ++i) {
                n2 *= n;
                n3 *= n + 1;
            }
            if (n2 <= this.entries && n3 > this.entries) {
                return n;
            }
            if (n2 > this.entries) {
                --n;
                continue;
            }
            ++n;
        }
    }

    void clear() {
    }

    float[] unquantize() {
        if (this.maptype == 1 || this.maptype == 2) {
            float f = StaticCodeBook.float32_unpack(this.q_min);
            float f2 = StaticCodeBook.float32_unpack(this.q_delta);
            float[] fArray = new float[this.entries * this.dim];
            switch (this.maptype) {
                case 1: {
                    int n = this.maptype1_quantvals();
                    for (int i = 0; i < this.entries; ++i) {
                        float f3 = 0.0f;
                        int n2 = 1;
                        for (int j = 0; j < this.dim; ++j) {
                            int n3 = i / n2 % n;
                            float f4 = this.quantlist[n3];
                            f4 = Math.abs(f4) * f2 + f + f3;
                            if (this.q_sequencep != 0) {
                                f3 = f4;
                            }
                            fArray[i * this.dim + j] = f4;
                            n2 *= n;
                        }
                    }
                    break;
                }
                case 2: {
                    for (int i = 0; i < this.entries; ++i) {
                        float f5 = 0.0f;
                        for (int j = 0; j < this.dim; ++j) {
                            float f6 = this.quantlist[i * this.dim + j];
                            f6 = Math.abs(f6) * f2 + f + f5;
                            if (this.q_sequencep != 0) {
                                f5 = f6;
                            }
                            fArray[i * this.dim + j] = f6;
                        }
                    }
                    break;
                }
            }
            return fArray;
        }
        return null;
    }

    static long float32_pack(float f) {
        int n = 0;
        if (f < 0.0f) {
            n = Integer.MIN_VALUE;
            f = -f;
        }
        int n2 = MathHelper.floor(Math.log(f) / Math.log(2.0));
        int n3 = (int)Math.rint(Math.pow(f, 20 - n2));
        n2 = n2 + 768 << 21;
        return n | n2 | n3;
    }

    static float float32_unpack(int i) {
        float f = i & 0x1FFFFF;
        float f2 = (i & 0x7FE00000) >>> 21;
        if ((i & Integer.MIN_VALUE) != 0) {
            f = -f;
        }
        return StaticCodeBook.ldexp(f, (int)f2 - 20 - 768);
    }

    static float ldexp(float f, int i) {
        return (float)((double)f * Math.pow(2.0, i));
    }
}

