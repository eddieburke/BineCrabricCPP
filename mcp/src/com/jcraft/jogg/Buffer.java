/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jogg;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class Buffer {
    private static final int BUFFER_INCREMENT = 256;
    private static final int[] mask = new int[]{0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, Short.MAX_VALUE, 65535, 131071, 262143, 524287, 1048575, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, Integer.MAX_VALUE, -1};
    int ptr = 0;
    byte[] buffer = null;
    int endbit = 0;
    int endbyte = 0;
    int storage = 0;

    public void writeinit() {
        this.buffer = new byte[256];
        this.ptr = 0;
        this.buffer[0] = 0;
        this.storage = 256;
    }

    public void write(byte[] bs) {
        for (int i = 0; i < bs.length && bs[i] != 0; ++i) {
            this.write(bs[i], 8);
        }
    }

    public void read(byte[] bs, int i) {
        int n = 0;
        while (i-- != 0) {
            bs[n++] = (byte)this.read(8);
        }
    }

    void reset() {
        this.ptr = 0;
        this.buffer[0] = 0;
        this.endbyte = 0;
        this.endbit = 0;
    }

    public void writeclear() {
        this.buffer = null;
    }

    public void readinit(byte[] bs, int i) {
        this.readinit(bs, 0, i);
    }

    public void readinit(byte[] bs, int i, int j) {
        this.ptr = i;
        this.buffer = bs;
        this.endbyte = 0;
        this.endbit = 0;
        this.storage = j;
    }

    public void write(int i, int j) {
        if (this.endbyte + 4 >= this.storage) {
            byte[] byArray = new byte[this.storage + 256];
            System.arraycopy(this.buffer, 0, byArray, 0, this.storage);
            this.buffer = byArray;
            this.storage += 256;
        }
        i &= mask[j];
        int n = this.ptr;
        this.buffer[n] = (byte)(this.buffer[n] | (byte)(i << this.endbit));
        if ((j += this.endbit) >= 8) {
            this.buffer[this.ptr + 1] = (byte)(i >>> 8 - this.endbit);
            if (j >= 16) {
                this.buffer[this.ptr + 2] = (byte)(i >>> 16 - this.endbit);
                if (j >= 24) {
                    this.buffer[this.ptr + 3] = (byte)(i >>> 24 - this.endbit);
                    if (j >= 32) {
                        this.buffer[this.ptr + 4] = this.endbit > 0 ? (byte)(i >>> 32 - this.endbit) : (byte)0;
                    }
                }
            }
        }
        this.endbyte += j / 8;
        this.ptr += j / 8;
        this.endbit = j & 7;
    }

    public int look(int i) {
        int n = mask[i];
        if (this.endbyte + 4 >= this.storage && this.endbyte + ((i += this.endbit) - 1) / 8 >= this.storage) {
            return -1;
        }
        int n2 = (this.buffer[this.ptr] & 0xFF) >>> this.endbit;
        if (i > 8) {
            n2 |= (this.buffer[this.ptr + 1] & 0xFF) << 8 - this.endbit;
            if (i > 16) {
                n2 |= (this.buffer[this.ptr + 2] & 0xFF) << 16 - this.endbit;
                if (i > 24) {
                    n2 |= (this.buffer[this.ptr + 3] & 0xFF) << 24 - this.endbit;
                    if (i > 32 && this.endbit != 0) {
                        n2 |= (this.buffer[this.ptr + 4] & 0xFF) << 32 - this.endbit;
                    }
                }
            }
        }
        return n & n2;
    }

    public int look1() {
        if (this.endbyte >= this.storage) {
            return -1;
        }
        return this.buffer[this.ptr] >> this.endbit & 1;
    }

    public void adv(int i) {
        this.ptr += (i += this.endbit) / 8;
        this.endbyte += i / 8;
        this.endbit = i & 7;
    }

    public void adv1() {
        ++this.endbit;
        if (this.endbit > 7) {
            this.endbit = 0;
            ++this.ptr;
            ++this.endbyte;
        }
    }

    public int read(int i) {
        int n;
        int n2 = mask[i];
        i += this.endbit;
        if (this.endbyte + 4 >= this.storage) {
            n = -1;
            if (this.endbyte + (i - 1) / 8 >= this.storage) {
                this.ptr += i / 8;
                this.endbyte += i / 8;
                this.endbit = i & 7;
                return n;
            }
        }
        n = (this.buffer[this.ptr] & 0xFF) >>> this.endbit;
        if (i > 8) {
            n |= (this.buffer[this.ptr + 1] & 0xFF) << 8 - this.endbit;
            if (i > 16) {
                n |= (this.buffer[this.ptr + 2] & 0xFF) << 16 - this.endbit;
                if (i > 24) {
                    n |= (this.buffer[this.ptr + 3] & 0xFF) << 24 - this.endbit;
                    if (i > 32 && this.endbit != 0) {
                        n |= (this.buffer[this.ptr + 4] & 0xFF) << 32 - this.endbit;
                    }
                }
            }
        }
        this.ptr += i / 8;
        this.endbyte += i / 8;
        this.endbit = i & 7;
        return n &= n2;
    }

    public int readB(int i) {
        int n;
        int n2 = 32 - i;
        i += this.endbit;
        if (this.endbyte + 4 >= this.storage) {
            n = -1;
            if (this.endbyte * 8 + i > this.storage * 8) {
                this.ptr += i / 8;
                this.endbyte += i / 8;
                this.endbit = i & 7;
                return n;
            }
        }
        n = (this.buffer[this.ptr] & 0xFF) << 24 + this.endbit;
        if (i > 8) {
            n |= (this.buffer[this.ptr + 1] & 0xFF) << 16 + this.endbit;
            if (i > 16) {
                n |= (this.buffer[this.ptr + 2] & 0xFF) << 8 + this.endbit;
                if (i > 24) {
                    n |= (this.buffer[this.ptr + 3] & 0xFF) << this.endbit;
                    if (i > 32 && this.endbit != 0) {
                        n |= (this.buffer[this.ptr + 4] & 0xFF) >> 8 - this.endbit;
                    }
                }
            }
        }
        n = n >>> (n2 >> 1) >>> (n2 + 1 >> 1);
        this.ptr += i / 8;
        this.endbyte += i / 8;
        this.endbit = i & 7;
        return n;
    }

    public int read1() {
        if (this.endbyte >= this.storage) {
            int n = -1;
            ++this.endbit;
            if (this.endbit > 7) {
                this.endbit = 0;
                ++this.ptr;
                ++this.endbyte;
            }
            return n;
        }
        int n = this.buffer[this.ptr] >> this.endbit & 1;
        ++this.endbit;
        if (this.endbit > 7) {
            this.endbit = 0;
            ++this.ptr;
            ++this.endbyte;
        }
        return n;
    }

    public int bytes() {
        return this.endbyte + (this.endbit + 7) / 8;
    }

    public int bits() {
        return this.endbyte * 8 + this.endbit;
    }

    public byte[] buffer() {
        return this.buffer;
    }

    public static int ilog(int i) {
        int n = 0;
        while (i > 0) {
            ++n;
            i >>>= 1;
        }
        return n;
    }

    public static void report(String string) {
        System.err.println(string);
        System.exit(1);
    }
}

