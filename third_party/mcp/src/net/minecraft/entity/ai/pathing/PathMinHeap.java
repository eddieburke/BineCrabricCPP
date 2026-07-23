/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.ai.pathing;

import net.minecraft.entity.ai.pathing.PathNode;

public class PathMinHeap {
    private PathNode[] pathNodes = new PathNode[1024];
    private int count = 0;

    public PathNode push(PathNode node) {
        if (node.heapIndex >= 0) {
            throw new IllegalStateException("OW KNOWS!");
        }
        if (this.count == this.pathNodes.length) {
            PathNode[] pathNodeArray = new PathNode[this.count << 1];
            System.arraycopy(this.pathNodes, 0, pathNodeArray, 0, this.count);
            this.pathNodes = pathNodeArray;
        }
        this.pathNodes[this.count] = node;
        node.heapIndex = this.count;
        this.shiftUp(this.count++);
        return node;
    }

    public void clear() {
        this.count = 0;
    }

    public PathNode pop() {
        PathNode pathNode = this.pathNodes[0];
        this.pathNodes[0] = this.pathNodes[--this.count];
        this.pathNodes[this.count] = null;
        if (this.count > 0) {
            this.shiftDown(0);
        }
        pathNode.heapIndex = -1;
        return pathNode;
    }

    public void setNodeWeight(PathNode node, float weight) {
        float f = node.heapWeight;
        node.heapWeight = weight;
        if (weight < f) {
            this.shiftUp(node.heapIndex);
        } else {
            this.shiftDown(node.heapIndex);
        }
    }

    private void shiftUp(int index) {
        PathNode pathNode = this.pathNodes[index];
        float f = pathNode.heapWeight;
        while (index > 0) {
            int n = index - 1 >> 1;
            PathNode pathNode2 = this.pathNodes[n];
            if (!(f < pathNode2.heapWeight)) break;
            this.pathNodes[index] = pathNode2;
            pathNode2.heapIndex = index;
            index = n;
        }
        this.pathNodes[index] = pathNode;
        pathNode.heapIndex = index;
    }

    private void shiftDown(int index) {
        PathNode pathNode = this.pathNodes[index];
        float f = pathNode.heapWeight;
        while (true) {
            float f2;
            PathNode pathNode2;
            int n = 1 + (index << 1);
            int n2 = n + 1;
            if (n >= this.count) break;
            PathNode pathNode3 = this.pathNodes[n];
            float f3 = pathNode3.heapWeight;
            if (n2 >= this.count) {
                pathNode2 = null;
                f2 = Float.POSITIVE_INFINITY;
            } else {
                pathNode2 = this.pathNodes[n2];
                f2 = pathNode2.heapWeight;
            }
            if (f3 < f2) {
                if (!(f3 < f)) break;
                this.pathNodes[index] = pathNode3;
                pathNode3.heapIndex = index;
                index = n;
                continue;
            }
            if (!(f2 < f)) break;
            this.pathNodes[index] = pathNode2;
            pathNode2.heapIndex = index;
            index = n2;
        }
        this.pathNodes[index] = pathNode;
        pathNode.heapIndex = index;
    }

    public boolean isEmpty() {
        return this.count == 0;
    }
}

