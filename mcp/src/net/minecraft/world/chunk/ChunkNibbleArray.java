/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk;

public class ChunkNibbleArray {
    public final byte[] bytes;

    public ChunkNibbleArray(int size) {
        this.bytes = new byte[size >> 1];
    }

    public ChunkNibbleArray(byte[] bytes) {
        this.bytes = bytes;
    }

    public int get(int x, int y, int z) {
        int n = x << 11 | z << 7 | y;
        int n2 = n >> 1;
        int n3 = n & 1;
        if (n3 == 0) {
            return this.bytes[n2] & 0xF;
        }
        return this.bytes[n2] >> 4 & 0xF;
    }

    public void set(int x, int y, int z, int value) {
        int n = x << 11 | z << 7 | y;
        int n2 = n >> 1;
        int n3 = n & 1;
        this.bytes[n2] = n3 == 0 ? (byte)(this.bytes[n2] & 0xF0 | value & 0xF) : (byte)(this.bytes[n2] & 0xF | (value & 0xF) << 4);
    }

    public boolean isArrayInitialized() {
        return this.bytes != null;
    }
}

