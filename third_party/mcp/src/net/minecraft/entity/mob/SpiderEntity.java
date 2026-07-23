/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.Entity;
import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class SpiderEntity
extends MonsterEntity {
    public SpiderEntity(World arg) {
        super(arg);
        this.texture = "/mob/spider.png";
        this.setBoundingBoxSpacing(1.4f, 0.9f);
        this.movementSpeed = 0.8f;
    }

    public double getPassengerRidingHeight() {
        return (double)this.height * 0.75 - 0.5;
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    protected Entity getTargetInRange() {
        float f = this.getBrightnessAtEyes(1.0f);
        if (f < 0.5f) {
            double d = 16.0;
            return this.world.getClosestPlayer(this, d);
        }
        return null;
    }

    protected String getRandomSound() {
        return "mob.spider";
    }

    protected String getHurtSound() {
        return "mob.spider";
    }

    protected String getDeathSound() {
        return "mob.spiderdeath";
    }

    protected void attack(Entity other, float distance) {
        float f = this.getBrightnessAtEyes(1.0f);
        if (f > 0.5f && this.random.nextInt(100) == 0) {
            this.target = null;
            return;
        }
        if (distance > 2.0f && distance < 6.0f && this.random.nextInt(10) == 0) {
            if (this.onGround) {
                double d = other.x - this.x;
                double d2 = other.z - this.z;
                float f2 = MathHelper.sqrt(d * d + d2 * d2);
                this.velocityX = d / (double)f2 * 0.5 * (double)0.8f + this.velocityX * (double)0.2f;
                this.velocityZ = d2 / (double)f2 * 0.5 * (double)0.8f + this.velocityZ * (double)0.2f;
                this.velocityY = 0.4f;
            }
        } else {
            super.attack(other, distance);
        }
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    protected int getDroppedItemId() {
        return Item.STRING.id;
    }

    public boolean isOnLadder() {
        return this.horizontalCollision;
    }
}

