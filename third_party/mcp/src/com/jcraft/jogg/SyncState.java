/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jogg;

import com.jcraft.jogg.Page;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class SyncState {
    public byte[] data;
    int storage;
    int fill;
    int returned;
    int unsynced;
    int headerbytes;
    int bodybytes;
    private Page pageseek = new Page();
    private byte[] chksum = new byte[4];

    public int clear() {
        this.data = null;
        return 0;
    }

    public int buffer(int i) {
        if (this.returned != 0) {
            this.fill -= this.returned;
            if (this.fill > 0) {
                System.arraycopy(this.data, this.returned, this.data, 0, this.fill);
            }
            this.returned = 0;
        }
        if (i > this.storage - this.fill) {
            int n = i + this.fill + 4096;
            if (this.data != null) {
                byte[] byArray = new byte[n];
                System.arraycopy(this.data, 0, byArray, 0, this.data.length);
                this.data = byArray;
            } else {
                this.data = new byte[n];
            }
            this.storage = n;
        }
        return this.fill;
    }

    public int wrote(int i) {
        if (this.fill + i > this.storage) {
            return -1;
        }
        this.fill += i;
        return 0;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public int pageseek(Page page) {
        int n = this.returned;
        int n2 = this.fill - this.returned;
        if (this.headerbytes == 0) {
            if (n2 < 27) {
                return 0;
            }
            if (this.data[n] != 79 || this.data[n + 1] != 103 || this.data[n + 2] != 103 || this.data[n + 3] != 83) {
                this.headerbytes = 0;
                this.bodybytes = 0;
                int n3 = 0;
                for (int i = 0; i < n2 - 1; ++i) {
                    if (this.data[n + 1 + i] != 79) continue;
                    n3 = n + 1 + i;
                    break;
                }
                if (n3 == 0) {
                    n3 = this.fill;
                }
                this.returned = n3;
                return -(n3 - n);
            }
            int n4 = (this.data[n + 26] & 0xFF) + 27;
            if (n2 < n4) {
                return 0;
            }
            for (int i = 0; i < (this.data[n + 26] & 0xFF); ++i) {
                this.bodybytes += this.data[n + 27 + i] & 0xFF;
            }
            this.headerbytes = n4;
        }
        if (this.bodybytes + this.headerbytes > n2) {
            return 0;
        }
        byte[] byArray = this.chksum;
        synchronized (this.chksum) {
            System.arraycopy(this.data, n + 22, this.chksum, 0, 4);
            this.data[n + 22] = 0;
            this.data[n + 23] = 0;
            this.data[n + 24] = 0;
            this.data[n + 25] = 0;
            Page page2 = this.pageseek;
            page2.header_base = this.data;
            page2.header = n;
            page2.header_len = this.headerbytes;
            page2.body_base = this.data;
            page2.body = n + this.headerbytes;
            page2.body_len = this.bodybytes;
            page2.checksum();
            if (this.chksum[0] != this.data[n + 22] || this.chksum[1] != this.data[n + 23] || this.chksum[2] != this.data[n + 24] || this.chksum[3] != this.data[n + 25]) {
                System.arraycopy(this.chksum, 0, this.data, n + 22, 4);
                this.headerbytes = 0;
                this.bodybytes = 0;
                int n5 = 0;
                for (int i = 0; i < n2 - 1; ++i) {
                    if (this.data[n + 1 + i] != 79) continue;
                    n5 = n + 1 + i;
                    break;
                }
                if (n5 == 0) {
                    n5 = this.fill;
                }
                this.returned = n5;
                // ** MonitorExit[var5_9] (shouldn't be in output)
                return -(n5 - n);
            }
            // ** MonitorExit[var5_9] (shouldn't be in output)
            n = this.returned;
            if (page != null) {
                page.header_base = this.data;
                page.header = n;
                page.header_len = this.headerbytes;
                page.body_base = this.data;
                page.body = n + this.headerbytes;
                page.body_len = this.bodybytes;
            }
            this.unsynced = 0;
            n2 = this.headerbytes + this.bodybytes;
            this.returned += n2;
            this.headerbytes = 0;
            this.bodybytes = 0;
            return n2;
        }
    }

    public int pageout(Page page) {
        do {
            int n;
            if ((n = this.pageseek(page)) > 0) {
                return 1;
            }
            if (n != 0) continue;
            return 0;
        } while (this.unsynced != 0);
        this.unsynced = 1;
        return -1;
    }

    public int reset() {
        this.fill = 0;
        this.returned = 0;
        this.unsynced = 0;
        this.headerbytes = 0;
        this.bodybytes = 0;
        return 0;
    }

    public void init() {
    }

    public int getDataOffset() {
        return this.returned;
    }

    public int getBufferOffset() {
        return this.fill;
    }
}

