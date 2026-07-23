/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util.hit;

import net.minecraft.entity.Entity;
import net.minecraft.util.hit.HitResultType;
import net.minecraft.util.math.Vec3d;

public class HitResult {
    public HitResultType type;
    public int blockX;
    public int blockY;
    public int blockZ;
    public int side;
    public Vec3d pos;
    public Entity entity;

    public HitResult(int blockX, int blockY, int blockZ, int side, Vec3d pos) {
        this.type = HitResultType.BLOCK;
        this.blockX = blockX;
        this.blockY = blockY;
        this.blockZ = blockZ;
        this.side = side;
        this.pos = Vec3d.createCached(pos.x, pos.y, pos.z);
    }

    public HitResult(Entity entity) {
        this.type = HitResultType.ENTITY;
        this.entity = entity;
        this.pos = Vec3d.createCached(entity.x, entity.y, entity.z);
    }
}

