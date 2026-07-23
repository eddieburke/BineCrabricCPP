/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.Entity;
import net.minecraft.entity.FlyingEntity;
import net.minecraft.entity.Monster;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.item.Item;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class GhastEntity
extends FlyingEntity
implements Monster {
    public int floatDuration = 0;
    public double targetX;
    public double targetY;
    public double targetZ;
    private Entity target = null;
    private int angerCooldown = 0;
    public int lastChargeTime = 0;
    public int chargeTime = 0;

    public GhastEntity(World arg) {
        super(arg);
        this.texture = "/mob/ghast.png";
        this.setBoundingBoxSpacing(4.0f, 4.0f);
        this.fireImmune = true;
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, (byte)0);
    }

    public void tick() {
        super.tick();
        byte by = this.dataTracker.getByte(16);
        this.texture = by == 1 ? "/mob/ghast_fire.png" : "/mob/ghast.png";
    }

    protected void tickLiving() {
        byte by;
        byte by2;
        if (!this.world.isRemote && this.world.difficulty == 0) {
            this.markDead();
        }
        this.tryDespawn();
        this.lastChargeTime = this.chargeTime;
        double d = this.targetX - this.x;
        double d2 = this.targetY - this.y;
        double d3 = this.targetZ - this.z;
        double d4 = MathHelper.sqrt(d * d + d2 * d2 + d3 * d3);
        if (d4 < 1.0 || d4 > 60.0) {
            this.targetX = this.x + (double)((this.random.nextFloat() * 2.0f - 1.0f) * 16.0f);
            this.targetY = this.y + (double)((this.random.nextFloat() * 2.0f - 1.0f) * 16.0f);
            this.targetZ = this.z + (double)((this.random.nextFloat() * 2.0f - 1.0f) * 16.0f);
        }
        if (this.floatDuration-- <= 0) {
            this.floatDuration += this.random.nextInt(5) + 2;
            if (this.canReach(this.targetX, this.targetY, this.targetZ, d4)) {
                this.velocityX += d / d4 * 0.1;
                this.velocityY += d2 / d4 * 0.1;
                this.velocityZ += d3 / d4 * 0.1;
            } else {
                this.targetX = this.x;
                this.targetY = this.y;
                this.targetZ = this.z;
            }
        }
        if (this.target != null && this.target.dead) {
            this.target = null;
        }
        if (this.target == null || this.angerCooldown-- <= 0) {
            this.target = this.world.getClosestPlayer(this, 100.0);
            if (this.target != null) {
                this.angerCooldown = 20;
            }
        }
        double d5 = 64.0;
        if (this.target != null && this.target.getSquaredDistance(this) < d5 * d5) {
            double d6 = this.target.x - this.x;
            double d7 = this.target.boundingBox.minY + (double)(this.target.height / 2.0f) - (this.y + (double)(this.height / 2.0f));
            double d8 = this.target.z - this.z;
            this.bodyYaw = this.yaw = -((float)Math.atan2(d6, d8)) * 180.0f / (float)Math.PI;
            if (this.canSee(this.target)) {
                if (this.chargeTime == 10) {
                    this.world.playSound(this, "mob.ghast.charge", this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
                }
                ++this.chargeTime;
                if (this.chargeTime == 20) {
                    this.world.playSound(this, "mob.ghast.fireball", this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
                    FireballEntity fireballEntity = new FireballEntity(this.world, this, d6, d7, d8);
                    double d9 = 4.0;
                    Vec3d vec3d = this.getLookVector(1.0f);
                    fireballEntity.x = this.x + vec3d.x * d9;
                    fireballEntity.y = this.y + (double)(this.height / 2.0f) + 0.5;
                    fireballEntity.z = this.z + vec3d.z * d9;
                    this.world.spawnEntity(fireballEntity);
                    this.chargeTime = -40;
                }
            } else if (this.chargeTime > 0) {
                --this.chargeTime;
            }
        } else {
            this.bodyYaw = this.yaw = -((float)Math.atan2(this.velocityX, this.velocityZ)) * 180.0f / (float)Math.PI;
            if (this.chargeTime > 0) {
                --this.chargeTime;
            }
        }
        if (!this.world.isRemote && (by2 = this.dataTracker.getByte(16)) != (by = (byte)(this.chargeTime > 10 ? 1 : 0))) {
            this.dataTracker.set(16, by);
        }
    }

    private boolean canReach(double x, double y, double z, double steps) {
        double d = (this.targetX - this.x) / steps;
        double d2 = (this.targetY - this.y) / steps;
        double d3 = (this.targetZ - this.z) / steps;
        Box box = this.boundingBox.copy();
        int n = 1;
        while ((double)n < steps) {
            box.translate(d, d2, d3);
            if (this.world.getEntityCollisions(this, box).size() > 0) {
                return false;
            }
            ++n;
        }
        return true;
    }

    protected String getRandomSound() {
        return "mob.ghast.moan";
    }

    protected String getHurtSound() {
        return "mob.ghast.scream";
    }

    protected String getDeathSound() {
        return "mob.ghast.death";
    }

    protected int getDroppedItemId() {
        return Item.GUNPOWDER.id;
    }

    protected float getSoundVolume() {
        return 10.0f;
    }

    public boolean canSpawn() {
        return this.random.nextInt(20) == 0 && super.canSpawn() && this.world.difficulty > 0;
    }

    public int getLimitPerChunk() {
        return 1;
    }
}

