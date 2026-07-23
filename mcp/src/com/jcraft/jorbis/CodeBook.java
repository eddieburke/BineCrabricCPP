/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jogg.Buffer;
import com.jcraft.jorbis.StaticCodeBook;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class CodeBook {
    int dim;
    int entries;
    StaticCodeBook c = new StaticCodeBook();
    float[] valuelist;
    int[] codelist;
    DecodeAux decode_tree;
    private int[] t = new int[15];

    CodeBook() {
    }

    int encode(int i, Buffer buffer) {
        buffer.write(this.codelist[i], this.c.lengthlist[i]);
        return this.c.lengthlist[i];
    }

    int errorv(float[] fs) {
        int n = this.best(fs, 1);
        for (int i = 0; i < this.dim; ++i) {
            fs[i] = this.valuelist[n * this.dim + i];
        }
        return n;
    }

    int encodev(int i, float[] fs, Buffer buffer) {
        for (int j = 0; j < this.dim; ++j) {
            fs[j] = this.valuelist[i * this.dim + j];
        }
        return this.encode(i, buffer);
    }

    int encodevs(float[] fs, Buffer buffer, int i, int j) {
        int n = this.besterror(fs, i, j);
        return this.encode(n, buffer);
    }

    synchronized int decodevs_add(float[] fs, int i, Buffer buffer, int j) {
        int n;
        int n2 = j / this.dim;
        if (this.t.length < n2) {
            this.t = new int[n2];
        }
        for (n = 0; n < n2; ++n) {
            int n3 = this.decode(buffer);
            if (n3 == -1) {
                return -1;
            }
            this.t[n] = n3 * this.dim;
        }
        n = 0;
        int n4 = 0;
        while (n < this.dim) {
            for (int k = 0; k < n2; ++k) {
                int n5 = i + n4 + k;
                fs[n5] = fs[n5] + this.valuelist[this.t[k] + n];
            }
            ++n;
            n4 += n2;
        }
        return 0;
    }

    int decodev_add(float[] fs, int i, Buffer buffer, int j) {
        if (this.dim > 8) {
            int n = 0;
            while (n < j) {
                int n2 = this.decode(buffer);
                if (n2 == -1) {
                    return -1;
                }
                int n3 = n2 * this.dim;
                int n4 = 0;
                while (n4 < this.dim) {
                    int n5 = i + n++;
                    fs[n5] = fs[n5] + this.valuelist[n3 + n4++];
                }
            }
        } else {
            int n = 0;
            while (n < j) {
                int n6 = this.decode(buffer);
                if (n6 == -1) {
                    return -1;
                }
                int n7 = n6 * this.dim;
                int n8 = 0;
                switch (this.dim) {
                    case 8: {
                        int n9 = i + n++;
                        fs[n9] = fs[n9] + this.valuelist[n7 + n8++];
                    }
                    case 7: {
                        int n10 = i + n++;
                        fs[n10] = fs[n10] + this.valuelist[n7 + n8++];
                    }
                    case 6: {
                        int n11 = i + n++;
                        fs[n11] = fs[n11] + this.valuelist[n7 + n8++];
                    }
                    case 5: {
                        int n12 = i + n++;
                        fs[n12] = fs[n12] + this.valuelist[n7 + n8++];
                    }
                    case 4: {
                        int n13 = i + n++;
                        fs[n13] = fs[n13] + this.valuelist[n7 + n8++];
                    }
                    case 3: {
                        int n14 = i + n++;
                        fs[n14] = fs[n14] + this.valuelist[n7 + n8++];
                    }
                    case 2: {
                        int n15 = i + n++;
                        fs[n15] = fs[n15] + this.valuelist[n7 + n8++];
                    }
                    case 1: {
                        int n16 = i + n++;
                        fs[n16] = fs[n16] + this.valuelist[n7 + n8++];
                    }
                }
            }
        }
        return 0;
    }

    int decodev_set(float[] fs, int i, Buffer buffer, int j) {
        int n = 0;
        while (n < j) {
            int n2 = this.decode(buffer);
            if (n2 == -1) {
                return -1;
            }
            int n3 = n2 * this.dim;
            int n4 = 0;
            while (n4 < this.dim) {
                fs[i + n++] = this.valuelist[n3 + n4++];
            }
        }
        return 0;
    }

    int decodevv_add(float[][] fs, int i, int j, Buffer buffer, int k) {
        int n = 0;
        int n2 = i / j;
        while (n2 < (i + k) / j) {
            int n3 = this.decode(buffer);
            if (n3 == -1) {
                return -1;
            }
            int n4 = n3 * this.dim;
            for (int i2 = 0; i2 < this.dim; ++i2) {
                float[] fArray = fs[n++];
                int n5 = n2++;
                fArray[n5] = fArray[n5] + this.valuelist[n4 + i2];
                if (n != j) continue;
                n = 0;
            }
        }
        return 0;
    }

    int decode(Buffer buffer) {
        int n = 0;
        DecodeAux decodeAux = this.decode_tree;
        int n2 = buffer.look(decodeAux.tabn);
        if (n2 >= 0) {
            n = decodeAux.tab[n2];
            buffer.adv(decodeAux.tabl[n2]);
            if (n <= 0) {
                return -n;
            }
        }
        do {
            switch (buffer.read1()) {
                case 0: {
                    n = decodeAux.ptr0[n];
                    break;
                }
                case 1: {
                    n = decodeAux.ptr1[n];
                    break;
                }
                default: {
                    return -1;
                }
            }
        } while (n > 0);
        return -n;
    }

    int decodevs(float[] fs, int i, Buffer buffer, int j, int k) {
        int n = this.decode(buffer);
        if (n == -1) {
            return -1;
        }
        switch (k) {
            case -1: {
                int n2 = 0;
                int n3 = 0;
                while (n2 < this.dim) {
                    fs[i + n3] = this.valuelist[n * this.dim + n2];
                    ++n2;
                    n3 += j;
                }
                break;
            }
            case 0: {
                int n4 = 0;
                int n5 = 0;
                while (n4 < this.dim) {
                    int n6 = i + n5;
                    fs[n6] = fs[n6] + this.valuelist[n * this.dim + n4];
                    ++n4;
                    n5 += j;
                }
                break;
            }
            case 1: {
                int n7 = 0;
                int n8 = 0;
                while (n7 < this.dim) {
                    int n9 = i + n8;
                    fs[n9] = fs[n9] * this.valuelist[n * this.dim + n7];
                    ++n7;
                    n8 += j;
                }
                break;
            }
        }
        return n;
    }

    int best(float[] fs, int i) {
        int n = -1;
        float f = 0.0f;
        int n2 = 0;
        for (int j = 0; j < this.entries; ++j) {
            if (this.c.lengthlist[j] > 0) {
                float f2 = CodeBook.dist(this.dim, this.valuelist, n2, fs, i);
                if (n == -1 || f2 < f) {
                    f = f2;
                    n = j;
                }
            }
            n2 += this.dim;
        }
        return n;
    }

    int besterror(float[] fs, int i, int j) {
        int n = this.best(fs, i);
        switch (j) {
            case 0: {
                int n2 = 0;
                int n3 = 0;
                while (n2 < this.dim) {
                    int n4 = n3;
                    fs[n4] = fs[n4] - this.valuelist[n * this.dim + n2];
                    ++n2;
                    n3 += i;
                }
                break;
            }
            case 1: {
                int n5 = 0;
                int n6 = 0;
                while (n5 < this.dim) {
                    float f = this.valuelist[n * this.dim + n5];
                    if (f == 0.0f) {
                        fs[n6] = 0.0f;
                    } else {
                        int n7 = n6;
                        fs[n7] = fs[n7] / f;
                    }
                    ++n5;
                    n6 += i;
                }
                break;
            }
        }
        return n;
    }

    void clear() {
    }

    private static float dist(int i, float[] fs, int j, float[] gs, int k) {
        float f = 0.0f;
        for (int i2 = 0; i2 < i; ++i2) {
            float f2 = fs[j + i2] - gs[i2 * k];
            f += f2 * f2;
        }
        return f;
    }

    int init_decode(StaticCodeBook staticCodeBook) {
        this.c = staticCodeBook;
        this.entries = staticCodeBook.entries;
        this.dim = staticCodeBook.dim;
        this.valuelist = staticCodeBook.unquantize();
        this.decode_tree = this.make_decode_tree();
        if (this.decode_tree == null) {
            this.clear();
            return -1;
        }
        return 0;
    }

    static int[] make_words(int[] is, int i) {
        int n;
        int n2;
        int n3;
        int[] nArray = new int[33];
        int[] nArray2 = new int[i];
        for (n3 = 0; n3 < i; ++n3) {
            n2 = is[n3];
            if (n2 <= 0) continue;
            n = nArray[n2];
            if (n2 < 32 && n >>> n2 != 0) {
                return null;
            }
            nArray2[n3] = n;
            int n4 = n2;
            while (n4 > 0) {
                if ((nArray[n4] & 1) != 0) {
                    if (n4 == 1) {
                        nArray[1] = nArray[1] + 1;
                        break;
                    }
                    nArray[n4] = nArray[n4 - 1] << 1;
                    break;
                }
                int n5 = n4--;
                nArray[n5] = nArray[n5] + 1;
            }
            for (n4 = n2 + 1; n4 < 33 && nArray[n4] >>> 1 == n; ++n4) {
                n = nArray[n4];
                nArray[n4] = nArray[n4 - 1] << 1;
            }
        }
        for (n3 = 0; n3 < i; ++n3) {
            n2 = 0;
            for (n = 0; n < is[n3]; ++n) {
                n2 <<= 1;
                n2 |= nArray2[n3] >>> n & 1;
            }
            nArray2[n3] = n2;
        }
        return nArray2;
    }

    DecodeAux make_decode_tree() {
        int n;
        int n2;
        int n3;
        int n4;
        int n5 = 0;
        DecodeAux decodeAux = new DecodeAux();
        decodeAux.ptr0 = new int[this.entries * 2];
        int[] nArray = decodeAux.ptr0;
        decodeAux.ptr1 = new int[this.entries * 2];
        int[] nArray2 = decodeAux.ptr1;
        int[] nArray3 = CodeBook.make_words(this.c.lengthlist, this.c.entries);
        if (nArray3 == null) {
            return null;
        }
        decodeAux.aux = this.entries * 2;
        for (n4 = 0; n4 < this.entries; ++n4) {
            if (this.c.lengthlist[n4] <= 0) continue;
            n3 = 0;
            for (n2 = 0; n2 < this.c.lengthlist[n4] - 1; ++n2) {
                n = nArray3[n4] >>> n2 & 1;
                if (n == 0) {
                    if (nArray[n3] == 0) {
                        nArray[n3] = ++n5;
                    }
                    n3 = nArray[n3];
                    continue;
                }
                if (nArray2[n3] == 0) {
                    nArray2[n3] = ++n5;
                }
                n3 = nArray2[n3];
            }
            if ((nArray3[n4] >>> n2 & 1) == 0) {
                nArray[n3] = -n4;
                continue;
            }
            nArray2[n3] = -n4;
        }
        decodeAux.tabn = Util.ilog(this.entries) - 4;
        if (decodeAux.tabn < 5) {
            decodeAux.tabn = 5;
        }
        n4 = 1 << decodeAux.tabn;
        decodeAux.tab = new int[n4];
        decodeAux.tabl = new int[n4];
        for (n3 = 0; n3 < n4; ++n3) {
            n2 = 0;
            n = 0;
            for (n = 0; n < decodeAux.tabn && (n2 > 0 || n == 0); ++n) {
                n2 = (n3 & 1 << n) != 0 ? nArray2[n2] : nArray[n2];
            }
            decodeAux.tab[n3] = n2;
            decodeAux.tabl[n3] = n;
        }
        return decodeAux;
    }

    @Environment(value=EnvType.CLIENT)
    class DecodeAux {
        int[] tab;
        int[] tabl;
        int tabn;
        int[] ptr0;
        int[] ptr1;
        int aux;

        DecodeAux() {
        }
    }
}

