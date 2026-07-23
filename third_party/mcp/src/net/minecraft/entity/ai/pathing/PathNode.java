/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.ai.pathing;

import net.minecraft.util.math.MathHelper;

public class PathNode {
    public final int x;
    public final int y;
    public final int z;
    private final int hashCode;
    int heapIndex = -1;
    float penalizedPathLength;
    float distanceToNearestTarget;
    float heapWeight;
    PathNode previous;
    public boolean visited = false;

    public PathNode(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.hashCode = PathNode.hash(x, y, z);
    }

    public static int hash(int x, int y, int z) {
        return y & 0xFF | (x & Short.MAX_VALUE) << 8 | (z & Short.MAX_VALUE) << 24 | (x < 0 ? Integer.MIN_VALUE : 0) | (z < 0 ? 32768 : 0);
    }

    public float getDistance(PathNode node) {
        float f = node.x - this.x;
        float f2 = node.y - this.y;
        float f3 = node.z - this.z;
        return MathHelper.sqrt(f * f + f2 * f2 + f3 * f3);
    }

    public boolean equals(Object object) {
        if (object instanceof PathNode) {
            PathNode pathNode = (PathNode)object;
            return this.hashCode == pathNode.hashCode && this.x == pathNode.x && this.y == pathNode.y && this.z == pathNode.z;
        }
        return false;
    }

    public int hashCode() {
        return this.hashCode;
    }

    public boolean isInHeap() {
        return this.heapIndex >= 0;
    }

    public String toString() {
        return this.x + ", " + this.y + ", " + this.z;
    }
}

