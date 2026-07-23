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
public class Page {
    private static int[] crc_lookup = new int[256];
    public byte[] header_base;
    public int header;
    public int header_len;
    public byte[] body_base;
    public int body;
    public int body_len;

    private static int crc_entry(int i) {
        int n = i << 24;
        for (int j = 0; j < 8; ++j) {
            if ((n & Integer.MIN_VALUE) != 0) {
                n = n << 1 ^ 0x4C11DB7;
                continue;
            }
            n <<= 1;
        }
        return n & 0xFFFFFFFF;
    }

    int version() {
        return this.header_base[this.header + 4] & 0xFF;
    }

    int continued() {
        return this.header_base[this.header + 5] & 1;
    }

    public int bos() {
        return this.header_base[this.header + 5] & 2;
    }

    public int eos() {
        return this.header_base[this.header + 5] & 4;
    }

    public long granulepos() {
        long l = this.header_base[this.header + 13] & 0xFF;
        l = l << 8 | (long)(this.header_base[this.header + 12] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 11] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 10] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 9] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 8] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 7] & 0xFF);
        l = l << 8 | (long)(this.header_base[this.header + 6] & 0xFF);
        return l;
    }

    public int serialno() {
        return this.header_base[this.header + 14] & 0xFF | (this.header_base[this.header + 15] & 0xFF) << 8 | (this.header_base[this.header + 16] & 0xFF) << 16 | (this.header_base[this.header + 17] & 0xFF) << 24;
    }

    int pageno() {
        return this.header_base[this.header + 18] & 0xFF | (this.header_base[this.header + 19] & 0xFF) << 8 | (this.header_base[this.header + 20] & 0xFF) << 16 | (this.header_base[this.header + 21] & 0xFF) << 24;
    }

    void checksum() {
        int n;
        int n2 = 0;
        for (n = 0; n < this.header_len; ++n) {
            n2 = n2 << 8 ^ crc_lookup[n2 >>> 24 & 0xFF ^ this.header_base[this.header + n] & 0xFF];
        }
        for (n = 0; n < this.body_len; ++n) {
            n2 = n2 << 8 ^ crc_lookup[n2 >>> 24 & 0xFF ^ this.body_base[this.body + n] & 0xFF];
        }
        this.header_base[this.header + 22] = (byte)n2;
        this.header_base[this.header + 23] = (byte)(n2 >>> 8);
        this.header_base[this.header + 24] = (byte)(n2 >>> 16);
        this.header_base[this.header + 25] = (byte)(n2 >>> 24);
    }

    public Page copy() {
        return this.copy(new Page());
    }

    public Page copy(Page page) {
        byte[] byArray = new byte[this.header_len];
        System.arraycopy(this.header_base, this.header, byArray, 0, this.header_len);
        page.header_len = this.header_len;
        page.header_base = byArray;
        page.header = 0;
        byArray = new byte[this.body_len];
        System.arraycopy(this.body_base, this.body, byArray, 0, this.body_len);
        page.body_len = this.body_len;
        page.body_base = byArray;
        page.body = 0;
        return page;
    }

    static {
        for (int i = 0; i < crc_lookup.length; ++i) {
            Page.crc_lookup[i] = Page.crc_entry(i);
        }
    }
}

