/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.item.Item;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class ZombieEntity
extends MonsterEntity {
    public ZombieEntity(World arg) {
        super(arg);
        this.texture = "/mob/zombie.png";
        this.movementSpeed = 0.5f;
        this.attackDamage = 5;
    }

    public void tickMovement() {
        float f;
        if (this.world.canMonsterSpawn() && (f = this.getBrightnessAtEyes(1.0f)) > 0.5f && this.world.hasSkyLight(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)) && this.random.nextFloat() * 30.0f < (f - 0.4f) * 2.0f) {
            this.fireTicks = 300;
        }
        super.tickMovement();
    }

    protected String getRandomSound() {
        return "mob.zombie";
    }

    protected String getHurtSound() {
        return "mob.zombiehurt";
    }

    protected String getDeathSound() {
        return "mob.zombiedeath";
    }

    protected int getDroppedItemId() {
        return Item.FEATHER.id;
    }
}

