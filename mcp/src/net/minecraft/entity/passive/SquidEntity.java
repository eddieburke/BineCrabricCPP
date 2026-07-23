/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.passive;

import net.minecraft.block.material.Material;
import net.minecraft.entity.WaterCreatureEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class SquidEntity
extends WaterCreatureEntity {
    public float tiltAngle = 0.0f;
    public float lastTiltAngle = 0.0f;
    public float rollAngle = 0.0f;
    public float lastRollAngle = 0.0f;
    public float thrustTimer = 0.0f;
    public float lastThrustTimer = 0.0f;
    public float tentacleAngle = 0.0f;
    public float lastTentacleAngle = 0.0f;
    private float swimVelocityScale = 0.0f;
    private float thrustTimerSpeed = 0.0f;
    private float turningSpeed = 0.0f;
    private float swimX = 0.0f;
    private float swimY = 0.0f;
    private float swimZ = 0.0f;

    public SquidEntity(World arg) {
        super(arg);
        this.texture = "/mob/squid.png";
        this.setBoundingBoxSpacing(0.95f, 0.95f);
        this.thrustTimerSpeed = 1.0f / (this.random.nextFloat() + 1.0f) * 0.2f;
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    protected String getRandomSound() {
        return null;
    }

    protected String getHurtSound() {
        return null;
    }

    protected String getDeathSound() {
        return null;
    }

    protected float getSoundVolume() {
        return 0.4f;
    }

    protected int getDroppedItemId() {
        return 0;
    }

    protected void dropItems() {
        int n = this.random.nextInt(3) + 1;
        for (int i = 0; i < n; ++i) {
            this.dropItem(new ItemStack(Item.DYE, 1, 0), 0.0f);
        }
    }

    public boolean interact(PlayerEntity player) {
        return false;
    }

    public boolean isSubmergedInWater() {
        return this.world.updateMovementInFluid(this.boundingBox.expand(0.0, -0.6f, 0.0), Material.WATER, this);
    }

    public void tickMovement() {
        super.tickMovement();
        this.lastTiltAngle = this.tiltAngle;
        this.lastRollAngle = this.rollAngle;
        this.lastThrustTimer = this.thrustTimer;
        this.lastTentacleAngle = this.tentacleAngle;
        this.thrustTimer += this.thrustTimerSpeed;
        if (this.thrustTimer > (float)Math.PI * 2) {
            this.thrustTimer -= (float)Math.PI * 2;
            if (this.random.nextInt(10) == 0) {
                this.thrustTimerSpeed = 1.0f / (this.random.nextFloat() + 1.0f) * 0.2f;
            }
        }
        if (this.isSubmergedInWater()) {
            float f;
            if (this.thrustTimer < (float)Math.PI) {
                f = this.thrustTimer / (float)Math.PI;
                this.tentacleAngle = MathHelper.sin(f * f * (float)Math.PI) * (float)Math.PI * 0.25f;
                if ((double)f > 0.75) {
                    this.swimVelocityScale = 1.0f;
                    this.turningSpeed = 1.0f;
                } else {
                    this.turningSpeed *= 0.8f;
                }
            } else {
                this.tentacleAngle = 0.0f;
                this.swimVelocityScale *= 0.9f;
                this.turningSpeed *= 0.99f;
            }
            if (!this.interpolateOnly) {
                this.velocityX = this.swimX * this.swimVelocityScale;
                this.velocityY = this.swimY * this.swimVelocityScale;
                this.velocityZ = this.swimZ * this.swimVelocityScale;
            }
            f = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
            this.bodyYaw += (-((float)Math.atan2(this.velocityX, this.velocityZ)) * 180.0f / (float)Math.PI - this.bodyYaw) * 0.1f;
            this.yaw = this.bodyYaw;
            this.rollAngle += (float)Math.PI * this.turningSpeed * 1.5f;
            this.tiltAngle += (-((float)Math.atan2(f, this.velocityY)) * 180.0f / (float)Math.PI - this.tiltAngle) * 0.1f;
        } else {
            this.tentacleAngle = MathHelper.abs(MathHelper.sin(this.thrustTimer)) * (float)Math.PI * 0.25f;
            if (!this.interpolateOnly) {
                this.velocityX = 0.0;
                this.velocityY -= 0.08;
                this.velocityY *= (double)0.98f;
                this.velocityZ = 0.0;
            }
            this.tiltAngle = (float)((double)this.tiltAngle + (double)(-90.0f - this.tiltAngle) * 0.02);
        }
    }

    public void travel(float x, float z) {
        this.move(this.velocityX, this.velocityY, this.velocityZ);
    }

    protected void tickLiving() {
        if (this.random.nextInt(50) == 0 || !this.submergedInWater || this.swimX == 0.0f && this.swimY == 0.0f && this.swimZ == 0.0f) {
            float f = this.random.nextFloat() * (float)Math.PI * 2.0f;
            this.swimX = MathHelper.cos(f) * 0.2f;
            this.swimY = -0.1f + this.random.nextFloat() * 0.2f;
            this.swimZ = MathHelper.sin(f) * 0.2f;
        }
        this.tryDespawn();
    }
}

