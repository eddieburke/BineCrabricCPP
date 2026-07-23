/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.passive;

import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class ChickenEntity
extends AnimalEntity {
    public boolean unused = false;
    public float flapProgress = 0.0f;
    public float maxWingDeviation = 0.0f;
    public float prevMaxWingDeviation;
    public float prevFlapProgress;
    public float flapSpeed = 1.0f;
    public int eggLayTime;

    public ChickenEntity(World arg) {
        super(arg);
        this.texture = "/mob/chicken.png";
        this.setBoundingBoxSpacing(0.3f, 0.4f);
        this.health = 4;
        this.eggLayTime = this.random.nextInt(6000) + 6000;
    }

    public void tickMovement() {
        super.tickMovement();
        this.prevFlapProgress = this.flapProgress;
        this.prevMaxWingDeviation = this.maxWingDeviation;
        this.maxWingDeviation = (float)((double)this.maxWingDeviation + (double)(this.onGround ? -1 : 4) * 0.3);
        if (this.maxWingDeviation < 0.0f) {
            this.maxWingDeviation = 0.0f;
        }
        if (this.maxWingDeviation > 1.0f) {
            this.maxWingDeviation = 1.0f;
        }
        if (!this.onGround && this.flapSpeed < 1.0f) {
            this.flapSpeed = 1.0f;
        }
        this.flapSpeed = (float)((double)this.flapSpeed * 0.9);
        if (!this.onGround && this.velocityY < 0.0) {
            this.velocityY *= 0.6;
        }
        this.flapProgress += this.flapSpeed * 2.0f;
        if (!this.world.isRemote && --this.eggLayTime <= 0) {
            this.world.playSound(this, "mob.chickenplop", 1.0f, (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
            this.dropItem(Item.EGG.id, 1);
            this.eggLayTime = this.random.nextInt(6000) + 6000;
        }
    }

    protected void onLanding(float fallDistance) {
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    protected String getRandomSound() {
        return "mob.chicken";
    }

    protected String getHurtSound() {
        return "mob.chickenhurt";
    }

    protected String getDeathSound() {
        return "mob.chickenhurt";
    }

    protected int getDroppedItemId() {
        return Item.FEATHER.id;
    }
}

