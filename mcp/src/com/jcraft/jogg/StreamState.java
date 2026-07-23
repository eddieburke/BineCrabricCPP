/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jogg;

import com.jcraft.jogg.Packet;
import com.jcraft.jogg.Page;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class StreamState {
    byte[] body_data;
    int body_storage;
    int body_fill;
    private int body_returned;
    int[] lacing_vals;
    long[] granule_vals;
    int lacing_storage;
    int lacing_fill;
    int lacing_packet;
    int lacing_returned;
    byte[] header = new byte[282];
    int header_fill;
    public int e_o_s;
    int b_o_s;
    int serialno;
    int pageno;
    long packetno;
    long granulepos;

    public StreamState() {
        this.init();
    }

    StreamState(int i) {
        this();
        this.init(i);
    }

    void init() {
        this.body_storage = 16384;
        this.body_data = new byte[this.body_storage];
        this.lacing_storage = 1024;
        this.lacing_vals = new int[this.lacing_storage];
        this.granule_vals = new long[this.lacing_storage];
    }

    public void init(int i) {
        if (this.body_data == null) {
            this.init();
        } else {
            int n;
            for (n = 0; n < this.body_data.length; ++n) {
                this.body_data[n] = 0;
            }
            for (n = 0; n < this.lacing_vals.length; ++n) {
                this.lacing_vals[n] = 0;
            }
            for (n = 0; n < this.granule_vals.length; ++n) {
                this.granule_vals[n] = 0L;
            }
        }
        this.serialno = i;
    }

    public void clear() {
        this.body_data = null;
        this.lacing_vals = null;
        this.granule_vals = null;
    }

    void destroy() {
        this.clear();
    }

    void body_expand(int i) {
        if (this.body_storage <= this.body_fill + i) {
            this.body_storage += i + 1024;
            byte[] byArray = new byte[this.body_storage];
            System.arraycopy(this.body_data, 0, byArray, 0, this.body_data.length);
            this.body_data = byArray;
        }
    }

    void lacing_expand(int i) {
        if (this.lacing_storage <= this.lacing_fill + i) {
            this.lacing_storage += i + 32;
            int[] nArray = new int[this.lacing_storage];
            System.arraycopy(this.lacing_vals, 0, nArray, 0, this.lacing_vals.length);
            this.lacing_vals = nArray;
            long[] lArray = new long[this.lacing_storage];
            System.arraycopy(this.granule_vals, 0, lArray, 0, this.granule_vals.length);
            this.granule_vals = lArray;
        }
    }

    public int packetin(Packet packet) {
        int n = packet.bytes / 255 + 1;
        if (this.body_returned != 0) {
            this.body_fill -= this.body_returned;
            if (this.body_fill != 0) {
                System.arraycopy(this.body_data, this.body_returned, this.body_data, 0, this.body_fill);
            }
            this.body_returned = 0;
        }
        this.body_expand(packet.bytes);
        this.lacing_expand(n);
        System.arraycopy(packet.packet_base, packet.packet, this.body_data, this.body_fill, packet.bytes);
        this.body_fill += packet.bytes;
        for (int i = 0; i < n - 1; ++i) {
            this.lacing_vals[this.lacing_fill + i] = 255;
            this.granule_vals[this.lacing_fill + i] = this.granulepos;
        }
        this.lacing_vals[this.lacing_fill + i] = packet.bytes % 255;
        long l = packet.granulepos;
        this.granule_vals[this.lacing_fill + i] = l;
        this.granulepos = l;
        int n2 = this.lacing_fill;
        this.lacing_vals[n2] = this.lacing_vals[n2] | 0x100;
        this.lacing_fill += n;
        ++this.packetno;
        if (packet.e_o_s != 0) {
            this.e_o_s = 1;
        }
        return 0;
    }

    public int packetout(Packet packet) {
        int n;
        if (this.lacing_packet <= (n = this.lacing_returned++)) {
            return 0;
        }
        if ((this.lacing_vals[n] & 0x400) != 0) {
            ++this.packetno;
            return -1;
        }
        int n2 = this.lacing_vals[n] & 0xFF;
        int n3 = 0;
        packet.packet_base = this.body_data;
        packet.packet = this.body_returned;
        packet.e_o_s = this.lacing_vals[n] & 0x200;
        packet.b_o_s = this.lacing_vals[n] & 0x100;
        n3 += n2;
        while (n2 == 255) {
            int n4 = this.lacing_vals[++n];
            n2 = n4 & 0xFF;
            if ((n4 & 0x200) != 0) {
                packet.e_o_s = 512;
            }
            n3 += n2;
        }
        packet.packetno = this.packetno++;
        packet.granulepos = this.granule_vals[n];
        packet.bytes = n3;
        this.body_returned += n3;
        this.lacing_returned = n + 1;
        return 1;
    }

    public int pagein(Page page) {
        byte[] byArray = page.header_base;
        int n = page.header;
        byte[] byArray2 = page.body_base;
        int n2 = page.body;
        int n3 = page.body_len;
        int n4 = 0;
        int n5 = page.version();
        int n6 = page.continued();
        int n7 = page.bos();
        int n8 = page.eos();
        long l = page.granulepos();
        int n9 = page.serialno();
        int n10 = page.pageno();
        int n11 = byArray[n + 26] & 0xFF;
        int n12 = this.lacing_returned;
        int n13 = this.body_returned;
        if (n13 != 0) {
            this.body_fill -= n13;
            if (this.body_fill != 0) {
                System.arraycopy(this.body_data, n13, this.body_data, 0, this.body_fill);
            }
            this.body_returned = 0;
        }
        if (n12 != 0) {
            if (this.lacing_fill - n12 != 0) {
                System.arraycopy(this.lacing_vals, n12, this.lacing_vals, 0, this.lacing_fill - n12);
                System.arraycopy(this.granule_vals, n12, this.granule_vals, 0, this.lacing_fill - n12);
            }
            this.lacing_fill -= n12;
            this.lacing_packet -= n12;
            this.lacing_returned = 0;
        }
        if (n9 != this.serialno) {
            return -1;
        }
        if (n5 > 0) {
            return -1;
        }
        this.lacing_expand(n11 + 1);
        if (n10 != this.pageno) {
            for (n12 = this.lacing_packet; n12 < this.lacing_fill; ++n12) {
                this.body_fill -= this.lacing_vals[n12] & 0xFF;
            }
            this.lacing_fill = this.lacing_packet++;
            if (this.pageno != -1) {
                this.lacing_vals[this.lacing_fill++] = 1024;
            }
            if (n6 != 0) {
                n7 = 0;
                while (n4 < n11) {
                    n13 = byArray[n + 27 + n4] & 0xFF;
                    n2 += n13;
                    n3 -= n13;
                    if (n13 < 255) {
                        ++n4;
                        break;
                    }
                    ++n4;
                }
            }
        }
        if (n3 != 0) {
            this.body_expand(n3);
            System.arraycopy(byArray2, n2, this.body_data, this.body_fill, n3);
            this.body_fill += n3;
        }
        n12 = -1;
        while (n4 < n11) {
            this.lacing_vals[this.lacing_fill] = n13 = byArray[n + 27 + n4] & 0xFF;
            this.granule_vals[this.lacing_fill] = -1L;
            if (n7 != 0) {
                int n14 = this.lacing_fill;
                this.lacing_vals[n14] = this.lacing_vals[n14] | 0x100;
                n7 = 0;
            }
            if (n13 < 255) {
                n12 = this.lacing_fill;
            }
            ++this.lacing_fill;
            ++n4;
            if (n13 >= 255) continue;
            this.lacing_packet = this.lacing_fill;
        }
        if (n12 != -1) {
            this.granule_vals[n12] = l;
        }
        if (n8 != 0) {
            this.e_o_s = 1;
            if (this.lacing_fill > 0) {
                int n15 = this.lacing_fill - 1;
                this.lacing_vals[n15] = this.lacing_vals[n15] | 0x200;
            }
        }
        this.pageno = n10 + 1;
        return 0;
    }

    public int flush(Page page) {
        int n;
        int n2 = 0;
        int n3 = this.lacing_fill > 255 ? 255 : this.lacing_fill;
        int n4 = 0;
        int n5 = 0;
        long l = this.granule_vals[0];
        if (n3 == 0) {
            return 0;
        }
        if (this.b_o_s == 0) {
            l = 0L;
            for (n2 = 0; n2 < n3; ++n2) {
                if ((this.lacing_vals[n2] & 0xFF) >= 255) continue;
                ++n2;
                break;
            }
        } else {
            for (n2 = 0; n2 < n3 && n5 <= 4096; n5 += this.lacing_vals[n2] & 0xFF, ++n2) {
                l = this.granule_vals[n2];
            }
        }
        System.arraycopy("OggS".getBytes(), 0, this.header, 0, 4);
        this.header[4] = 0;
        this.header[5] = 0;
        if ((this.lacing_vals[0] & 0x100) == 0) {
            this.header[5] = (byte)(this.header[5] | 1);
        }
        if (this.b_o_s == 0) {
            this.header[5] = (byte)(this.header[5] | 2);
        }
        if (this.e_o_s != 0 && this.lacing_fill == n2) {
            this.header[5] = (byte)(this.header[5] | 4);
        }
        this.b_o_s = 1;
        for (n = 6; n < 14; ++n) {
            this.header[n] = (byte)l;
            l >>>= 8;
        }
        int n6 = this.serialno;
        for (n = 14; n < 18; ++n) {
            this.header[n] = (byte)n6;
            n6 >>>= 8;
        }
        if (this.pageno == -1) {
            this.pageno = 0;
        }
        n6 = this.pageno++;
        for (n = 18; n < 22; ++n) {
            this.header[n] = (byte)n6;
            n6 >>>= 8;
        }
        this.header[22] = 0;
        this.header[23] = 0;
        this.header[24] = 0;
        this.header[25] = 0;
        this.header[26] = (byte)n2;
        for (n = 0; n < n2; ++n) {
            this.header[n + 27] = (byte)this.lacing_vals[n];
            n4 += this.header[n + 27] & 0xFF;
        }
        page.header_base = this.header;
        page.header = 0;
        page.header_len = this.header_fill = n2 + 27;
        page.body_base = this.body_data;
        page.body = this.body_returned;
        page.body_len = n4;
        this.lacing_fill -= n2;
        System.arraycopy(this.lacing_vals, n2, this.lacing_vals, 0, this.lacing_fill * 4);
        System.arraycopy(this.granule_vals, n2, this.granule_vals, 0, this.lacing_fill * 8);
        this.body_returned += n4;
        page.checksum();
        return 1;
    }

    public int pageout(Page page) {
        if (this.e_o_s != 0 && this.lacing_fill != 0 || this.body_fill - this.body_returned > 4096 || this.lacing_fill >= 255 || this.lacing_fill != 0 && this.b_o_s == 0) {
            return this.flush(page);
        }
        return 0;
    }

    public int eof() {
        return this.e_o_s;
    }

    public int reset() {
        this.body_fill = 0;
        this.body_returned = 0;
        this.lacing_fill = 0;
        this.lacing_packet = 0;
        this.lacing_returned = 0;
        this.header_fill = 0;
        this.e_o_s = 0;
        this.b_o_s = 0;
        this.pageno = -1;
        this.packetno = 0L;
        this.granulepos = 0L;
        return 0;
    }
}

