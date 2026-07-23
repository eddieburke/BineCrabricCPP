/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.ai.pathing;

import net.minecraft.entity.Entity;
import net.minecraft.entity.ai.pathing.PathNode;
import net.minecraft.util.math.Vec3d;

public class Path {
    private final PathNode[] nodes;
    public final int length;
    private int currentNodeIndex;

    public Path(PathNode[] nodes) {
        this.nodes = nodes;
        this.length = nodes.length;
    }

    public void next() {
        ++this.currentNodeIndex;
    }

    public boolean isFinished() {
        return this.currentNodeIndex >= this.nodes.length;
    }

    public PathNode getEnd() {
        if (this.length > 0) {
            return this.nodes[this.length - 1];
        }
        return null;
    }

    public Vec3d getNodePosition(Entity entity) {
        double d = (double)this.nodes[this.currentNodeIndex].x + (double)((int)(entity.width + 1.0f)) * 0.5;
        double d2 = this.nodes[this.currentNodeIndex].y;
        double d3 = (double)this.nodes[this.currentNodeIndex].z + (double)((int)(entity.width + 1.0f)) * 0.5;
        return Vec3d.createCached(d, d2, d3);
    }
}

