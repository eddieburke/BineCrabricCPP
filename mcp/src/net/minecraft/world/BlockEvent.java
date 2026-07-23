/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world;

public class BlockEvent
implements Comparable {
    private static long counter = 0L;
    public int x;
    public int y;
    public int z;
    public int blockId;
    public long ticks;
    private long globalId = counter++;

    public BlockEvent(int x, int y, int z, int blockId) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.blockId = blockId;
    }

    public boolean equals(Object other) {
        if (other instanceof BlockEvent) {
            BlockEvent blockEvent = (BlockEvent)other;
            return this.x == blockEvent.x && this.y == blockEvent.y && this.z == blockEvent.z && this.blockId == blockEvent.blockId;
        }
        return false;
    }

    public int hashCode() {
        return (this.x * 128 * 1024 + this.z * 128 + this.y) * 256 + this.blockId;
    }

    public BlockEvent get(long ticks) {
        this.ticks = ticks;
        return this;
    }

    public int compareTo(BlockEvent arg) {
        if (this.ticks < arg.ticks) {
            return -1;
        }
        if (this.ticks > arg.ticks) {
            return 1;
        }
        if (this.globalId < arg.globalId) {
            return -1;
        }
        if (this.globalId > arg.globalId) {
            return 1;
        }
        return 0;
    }
}

