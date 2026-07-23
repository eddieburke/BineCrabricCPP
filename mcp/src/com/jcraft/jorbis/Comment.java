/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jogg.Buffer;
import com.jcraft.jogg.Packet;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class Comment {
    private static byte[] _vorbis = "vorbis".getBytes();
    private static byte[] _vendor = "Xiphophorus libVorbis I 20000508".getBytes();
    private static final int OV_EIMPL = -130;
    public byte[][] user_comments;
    public int[] comment_lengths;
    public int comments;
    public byte[] vendor;

    public void init() {
        this.user_comments = null;
        this.comments = 0;
        this.vendor = null;
    }

    public void add(String string) {
        this.add(string.getBytes());
    }

    private void add(byte[] bs) {
        byte[][] byArrayArray = new byte[this.comments + 2][];
        if (this.user_comments != null) {
            System.arraycopy(this.user_comments, 0, byArrayArray, 0, this.comments);
        }
        this.user_comments = byArrayArray;
        int[] nArray = new int[this.comments + 2];
        if (this.comment_lengths != null) {
            System.arraycopy(this.comment_lengths, 0, nArray, 0, this.comments);
        }
        this.comment_lengths = nArray;
        byte[] byArray = new byte[bs.length + 1];
        System.arraycopy(bs, 0, byArray, 0, bs.length);
        this.user_comments[this.comments] = byArray;
        this.comment_lengths[this.comments] = bs.length;
        ++this.comments;
        this.user_comments[this.comments] = null;
    }

    public void add_tag(String string, String string2) {
        if (string2 == null) {
            string2 = "";
        }
        this.add(string + "=" + string2);
    }

    static boolean tagcompare(byte[] bs, byte[] cs, int i) {
        for (int j = 0; j < i; ++j) {
            byte by = bs[j];
            byte by2 = cs[j];
            if (90 >= by && by >= 65) {
                by = (byte)(by - 65 + 97);
            }
            if (90 >= by2 && by2 >= 65) {
                by2 = (byte)(by2 - 65 + 97);
            }
            if (by == by2) continue;
            return false;
        }
        return true;
    }

    public String query(String string) {
        return this.query(string, 0);
    }

    public String query(String string, int i) {
        int n = this.query(string.getBytes(), i);
        if (n == -1) {
            return null;
        }
        byte[] byArray = this.user_comments[n];
        for (int j = 0; j < this.comment_lengths[n]; ++j) {
            if (byArray[j] != 61) continue;
            return new String(byArray, j + 1, this.comment_lengths[n] - (j + 1));
        }
        return null;
    }

    private int query(byte[] bs, int i) {
        int n = 0;
        int n2 = 0;
        int n3 = bs.length + 1;
        byte[] byArray = new byte[n3];
        System.arraycopy(bs, 0, byArray, 0, bs.length);
        byArray[bs.length] = 61;
        for (n = 0; n < this.comments; ++n) {
            if (!Comment.tagcompare(this.user_comments[n], byArray, n3)) continue;
            if (i == n2) {
                return n;
            }
            ++n2;
        }
        return -1;
    }

    int unpack(Buffer buffer) {
        int n = buffer.read(32);
        if (n < 0) {
            this.clear();
            return -1;
        }
        this.vendor = new byte[n + 1];
        buffer.read(this.vendor, n);
        this.comments = buffer.read(32);
        if (this.comments < 0) {
            this.clear();
            return -1;
        }
        this.user_comments = new byte[this.comments + 1][];
        this.comment_lengths = new int[this.comments + 1];
        for (int i = 0; i < this.comments; ++i) {
            int n2 = buffer.read(32);
            if (n2 < 0) {
                this.clear();
                return -1;
            }
            this.comment_lengths[i] = n2;
            this.user_comments[i] = new byte[n2 + 1];
            buffer.read(this.user_comments[i], n2);
        }
        if (buffer.read(1) != 1) {
            this.clear();
            return -1;
        }
        return 0;
    }

    int pack(Buffer buffer) {
        buffer.write(3, 8);
        buffer.write(_vorbis);
        buffer.write(_vendor.length, 32);
        buffer.write(_vendor);
        buffer.write(this.comments, 32);
        if (this.comments != 0) {
            for (int i = 0; i < this.comments; ++i) {
                if (this.user_comments[i] != null) {
                    buffer.write(this.comment_lengths[i], 32);
                    buffer.write(this.user_comments[i]);
                    continue;
                }
                buffer.write(0, 32);
            }
        }
        buffer.write(1, 1);
        return 0;
    }

    public int header_out(Packet packet) {
        Buffer buffer = new Buffer();
        buffer.writeinit();
        if (this.pack(buffer) != 0) {
            return -130;
        }
        packet.packet_base = new byte[buffer.bytes()];
        packet.packet = 0;
        packet.bytes = buffer.bytes();
        System.arraycopy(buffer.buffer(), 0, packet.packet_base, 0, packet.bytes);
        packet.b_o_s = 0;
        packet.e_o_s = 0;
        packet.granulepos = 0L;
        return 0;
    }

    void clear() {
        for (int i = 0; i < this.comments; ++i) {
            this.user_comments[i] = null;
        }
        this.user_comments = null;
        this.vendor = null;
    }

    public String getVendor() {
        return new String(this.vendor, 0, this.vendor.length - 1);
    }

    public String getComment(int i) {
        if (this.comments <= i) {
            return null;
        }
        return new String(this.user_comments[i], 0, this.user_comments[i].length - 1);
    }

    public String toString() {
        String string = "Vendor: " + new String(this.vendor, 0, this.vendor.length - 1);
        for (int i = 0; i < this.comments; ++i) {
            string = string + "\nComment: " + new String(this.user_comments[i], 0, this.user_comments[i].length - 1);
        }
        string = string + "\n";
        return string;
    }
}

