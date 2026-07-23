/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.math;

public class ChunkPos {
    public final int x;
    public final int z;

    public ChunkPos(int x, int z) {
        this.x = x;
        this.z = z;
    }

    public static int hashCode(int x, int z) {
        return (x < 0 ? Integer.MIN_VALUE : 0) | (x & Short.MAX_VALUE) << 16 | (z < 0 ? 32768 : 0) | z & Short.MAX_VALUE;
    }

    public int hashCode() {
        return ChunkPos.hashCode(this.x, this.z);
    }

    public boolean equals(Object o) {
        ChunkPos chunkPos = (ChunkPos)o;
        return chunkPos.x == this.x && chunkPos.z == this.z;
    }
}

