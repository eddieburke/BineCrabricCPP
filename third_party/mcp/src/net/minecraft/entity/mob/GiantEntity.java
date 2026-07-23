/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.world.World;

public class GiantEntity
extends MonsterEntity {
    public GiantEntity(World arg) {
        super(arg);
        this.texture = "/mob/zombie.png";
        this.movementSpeed = 0.5f;
        this.attackDamage = 50;
        this.health *= 10;
        this.standingEyeHeight *= 6.0f;
        this.setBoundingBoxSpacing(this.width * 6.0f, this.height * 6.0f);
    }

    protected float getPathfindingFavor(int x, int y, int z) {
        return this.world.getLightBrightness(x, y, z) - 0.5f;
    }
}

