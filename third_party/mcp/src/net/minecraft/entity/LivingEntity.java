/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.sound.BlockSoundGroup;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public abstract class LivingEntity
extends Entity {
    public int maxHealth = 20;
    public float randomWalkYaw;
    public float randomPitch;
    public float bodyYaw = 0.0f;
    public float lastBodyYaw = 0.0f;
    protected float lastWalkProgress;
    protected float walkProgress;
    protected float bodyYaw;
    protected float prevBodyYaw;
    protected boolean collisionFlag = true;
    protected String texture = "/mob/char.png";
    protected boolean gravityEnabled = true;
    protected float rotationOffset = 0.0f;
    protected String modelName = null;
    protected float entityScale = 1.0f;
    protected int scoreAmount = 0;
    protected float damageAmount = 0.0f;
    public boolean interpolateOnly = false;
    public float lastSwingAnimationProgress;
    public float swingAnimationProgress;
    public int health = 10;
    public int lastHealth;
    private int ambientSoundTimer;
    public int hurtTime;
    public int damagedTime;
    public float damagedSwingDir = 0.0f;
    public int deathTime = 0;
    public int attackCooldown = 0;
    public float prevTilt;
    public float tilt;
    protected boolean killedByOtherEntity = false;
    public int ambientSoundId = -1;
    public float ambientSoundInterval = (float)(Math.random() * (double)0.9f + (double)0.1f);
    public float lastWalkAnimationSpeed;
    public float walkAnimationSpeed;
    public float walkAnimationProgress;
    protected int bodyTrackingIncrements;
    protected double lerpX;
    protected double lerpY;
    protected double lerpZ;
    protected double lerpYaw;
    protected double lerpPitch;
    float animationTimer = 0.0f;
    protected int prevHealth = 0;
    protected int despawnCounter = 0;
    protected float sidewaysSpeed;
    protected float forwardSpeed;
    protected float rotationSpeed;
    protected boolean jumping = false;
    protected float defaultPitch = 0.0f;
    protected float movementSpeed = 0.7f;
    private Entity lookTarget;
    protected int lookTimer = 0;

    public LivingEntity(World arg) {
        super(arg);
        this.blocksSameBlockSpawning = true;
        this.randomPitch = (float)(Math.random() + 1.0) * 0.01f;
        this.setPosition(this.x, this.y, this.z);
        this.randomWalkYaw = (float)Math.random() * 12398.0f;
        this.yaw = (float)(Math.random() * 3.1415927410125732 * 2.0);
        this.stepHeight = 0.5f;
    }

    protected void initDataTracker() {
    }

    public boolean canSee(Entity entity) {
        return this.world.raycast(Vec3d.createCached(this.x, this.y + (double)this.getEyeHeight(), this.z), Vec3d.createCached(entity.x, entity.y + (double)entity.getEyeHeight(), entity.z)) == null;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTexture() {
        return this.texture;
    }

    public boolean isCollidable() {
        return !this.dead;
    }

    public boolean isPushable() {
        return !this.dead;
    }

    public float getEyeHeight() {
        return this.height * 0.85f;
    }

    public int getMinAmbientSoundDelay() {
        return 80;
    }

    public void makeSound() {
        String string = this.getRandomSound();
        if (string != null) {
            this.world.playSound(this, string, this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
        }
    }

    public void baseTick() {
        int n;
        this.lastSwingAnimationProgress = this.swingAnimationProgress;
        super.baseTick();
        if (this.random.nextInt(1000) < this.ambientSoundTimer++) {
            this.ambientSoundTimer = -this.getMinAmbientSoundDelay();
            this.makeSound();
        }
        if (this.isAlive() && this.isInsideWall()) {
            this.damage(null, 1);
        }
        if (this.fireImmune || this.world.isRemote) {
            this.fireTicks = 0;
        }
        if (this.isAlive() && this.isInFluid(Material.WATER) && !this.canBreatheInWater()) {
            --this.air;
            if (this.air == -20) {
                this.air = 0;
                for (n = 0; n < 8; ++n) {
                    float f = this.random.nextFloat() - this.random.nextFloat();
                    float f2 = this.random.nextFloat() - this.random.nextFloat();
                    float f3 = this.random.nextFloat() - this.random.nextFloat();
                    this.world.addParticle("bubble", this.x + (double)f, this.y + (double)f2, this.z + (double)f3, this.velocityX, this.velocityY, this.velocityZ);
                }
                this.damage(null, 2);
            }
            this.fireTicks = 0;
        } else {
            this.air = this.maxAir;
        }
        this.prevTilt = this.tilt;
        if (this.attackCooldown > 0) {
            --this.attackCooldown;
        }
        if (this.hurtTime > 0) {
            --this.hurtTime;
        }
        if (this.hearts > 0) {
            --this.hearts;
        }
        if (this.health <= 0) {
            ++this.deathTime;
            if (this.deathTime > 20) {
                this.beforeRemove();
                this.markDead();
                for (n = 0; n < 20; ++n) {
                    double d = this.random.nextGaussian() * 0.02;
                    double d2 = this.random.nextGaussian() * 0.02;
                    double d3 = this.random.nextGaussian() * 0.02;
                    this.world.addParticle("explode", this.x + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width, this.y + (double)(this.random.nextFloat() * this.height), this.z + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width, d, d2, d3);
                }
            }
        }
        this.prevBodyYaw = this.bodyYaw;
        this.lastBodyYaw = this.bodyYaw;
        this.prevYaw = this.yaw;
        this.prevPitch = this.pitch;
    }

    public void animateSpawn() {
        for (int i = 0; i < 20; ++i) {
            double d = this.random.nextGaussian() * 0.02;
            double d2 = this.random.nextGaussian() * 0.02;
            double d3 = this.random.nextGaussian() * 0.02;
            double d4 = 10.0;
            this.world.addParticle("explode", this.x + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width - d * d4, this.y + (double)(this.random.nextFloat() * this.height) - d2 * d4, this.z + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width - d3 * d4, d, d2, d3);
        }
    }

    public void tickRiding() {
        super.tickRiding();
        this.lastWalkProgress = this.walkProgress;
        this.walkProgress = 0.0f;
    }

    @Environment(value=EnvType.CLIENT)
    public void setPositionAndAnglesAvoidEntities(double x, double y, double z, float pitch, float yaw, int interpolationSteps) {
        this.standingEyeHeight = 0.0f;
        this.lerpX = x;
        this.lerpY = y;
        this.lerpZ = z;
        this.lerpYaw = pitch;
        this.lerpPitch = yaw;
        this.bodyTrackingIncrements = interpolationSteps;
    }

    public void tick() {
        boolean bl;
        float f;
        float f2;
        super.tick();
        this.tickMovement();
        double d = this.x - this.prevX;
        double d2 = this.z - this.prevZ;
        float f3 = MathHelper.sqrt(d * d + d2 * d2);
        float f4 = this.bodyYaw;
        float f5 = 0.0f;
        this.lastWalkProgress = this.walkProgress;
        float f6 = 0.0f;
        if (!(f3 <= 0.05f)) {
            f6 = 1.0f;
            f5 = f3 * 3.0f;
            f4 = (float)Math.atan2(d2, d) * 180.0f / (float)Math.PI - 90.0f;
        }
        if (this.swingAnimationProgress > 0.0f) {
            f4 = this.yaw;
        }
        if (!this.onGround) {
            f6 = 0.0f;
        }
        this.walkProgress += (f6 - this.walkProgress) * 0.3f;
        for (f2 = f4 - this.bodyYaw; f2 < -180.0f; f2 += 360.0f) {
        }
        while (f2 >= 180.0f) {
            f2 -= 360.0f;
        }
        this.bodyYaw += f2 * 0.3f;
        for (f = this.yaw - this.bodyYaw; f < -180.0f; f += 360.0f) {
        }
        while (f >= 180.0f) {
            f -= 360.0f;
        }
        boolean bl2 = bl = f < -90.0f || f >= 90.0f;
        if (f < -75.0f) {
            f = -75.0f;
        }
        if (f >= 75.0f) {
            f = 75.0f;
        }
        this.bodyYaw = this.yaw - f;
        if (f * f > 2500.0f) {
            this.bodyYaw += f * 0.2f;
        }
        if (bl) {
            f5 *= -1.0f;
        }
        while (this.yaw - this.prevYaw < -180.0f) {
            this.prevYaw -= 360.0f;
        }
        while (this.yaw - this.prevYaw >= 180.0f) {
            this.prevYaw += 360.0f;
        }
        while (this.bodyYaw - this.lastBodyYaw < -180.0f) {
            this.lastBodyYaw -= 360.0f;
        }
        while (this.bodyYaw - this.lastBodyYaw >= 180.0f) {
            this.lastBodyYaw += 360.0f;
        }
        while (this.pitch - this.prevPitch < -180.0f) {
            this.prevPitch -= 360.0f;
        }
        while (this.pitch - this.prevPitch >= 180.0f) {
            this.prevPitch += 360.0f;
        }
        this.bodyYaw += f5;
    }

    protected void setBoundingBoxSpacing(float spacingXZ, float spacingY) {
        super.setBoundingBoxSpacing(spacingXZ, spacingY);
    }

    public void heal(int amount) {
        if (this.health <= 0) {
            return;
        }
        this.health += amount;
        if (this.health > 20) {
            this.health = 20;
        }
        this.hearts = this.maxHealth / 2;
    }

    public boolean damage(Entity damageSource, int amount) {
        if (this.world.isRemote) {
            return false;
        }
        this.despawnCounter = 0;
        if (this.health <= 0) {
            return false;
        }
        this.walkAnimationSpeed = 1.5f;
        boolean bl = true;
        if ((float)this.hearts > (float)this.maxHealth / 2.0f) {
            if (amount <= this.prevHealth) {
                return false;
            }
            this.applyDamage(amount - this.prevHealth);
            this.prevHealth = amount;
            bl = false;
        } else {
            this.prevHealth = amount;
            this.lastHealth = this.health;
            this.hearts = this.maxHealth;
            this.applyDamage(amount);
            this.damagedTime = 10;
            this.hurtTime = 10;
        }
        this.damagedSwingDir = 0.0f;
        if (bl) {
            this.world.broadcastEntityEvent(this, (byte)2);
            this.scheduleVelocityUpdate();
            if (damageSource != null) {
                double d = damageSource.x - this.x;
                double d2 = damageSource.z - this.z;
                while (d * d + d2 * d2 < 1.0E-4) {
                    d = (Math.random() - Math.random()) * 0.01;
                    d2 = (Math.random() - Math.random()) * 0.01;
                }
                this.damagedSwingDir = (float)(Math.atan2(d2, d) * 180.0 / 3.1415927410125732) - this.yaw;
                this.applyKnockback(damageSource, amount, d, d2);
            } else {
                this.damagedSwingDir = (int)(Math.random() * 2.0) * 180;
            }
        }
        if (this.health <= 0) {
            if (bl) {
                this.world.playSound(this, this.getDeathSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
            }
            this.onKilledBy(damageSource);
        } else if (bl) {
            this.world.playSound(this, this.getHurtSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
        }
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public void animateHurt() {
        this.damagedTime = 10;
        this.hurtTime = 10;
        this.damagedSwingDir = 0.0f;
    }

    protected void applyDamage(int amount) {
        this.health -= amount;
    }

    protected float getSoundVolume() {
        return 1.0f;
    }

    protected String getRandomSound() {
        return null;
    }

    protected String getHurtSound() {
        return "random.hurt";
    }

    protected String getDeathSound() {
        return "random.hurt";
    }

    public void applyKnockback(Entity attacker, int amount, double dx, double dz) {
        float f = MathHelper.sqrt(dx * dx + dz * dz);
        float f2 = 0.4f;
        this.velocityX /= 2.0;
        this.velocityY /= 2.0;
        this.velocityZ /= 2.0;
        this.velocityX -= dx / (double)f * (double)f2;
        this.velocityY += (double)0.4f;
        this.velocityZ -= dz / (double)f * (double)f2;
        if (this.velocityY > (double)0.4f) {
            this.velocityY = 0.4f;
        }
    }

    public void onKilledBy(Entity adversary) {
        if (this.scoreAmount >= 0 && adversary != null) {
            adversary.updateKilledAchievement(this, this.scoreAmount);
        }
        if (adversary != null) {
            adversary.onKilledOther(this);
        }
        this.killedByOtherEntity = true;
        if (!this.world.isRemote) {
            this.dropItems();
        }
        this.world.broadcastEntityEvent(this, (byte)3);
    }

    protected void dropItems() {
        int n = this.getDroppedItemId();
        if (n > 0) {
            int n2 = this.random.nextInt(3);
            for (int i = 0; i < n2; ++i) {
                this.dropItem(n, 1);
            }
        }
    }

    protected int getDroppedItemId() {
        return 0;
    }

    protected void onLanding(float fallDistance) {
        super.onLanding(fallDistance);
        int n = (int)Math.ceil(fallDistance - 3.0f);
        if (n > 0) {
            this.damage(null, n);
            int n2 = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.y - (double)0.2f - (double)this.standingEyeHeight), MathHelper.floor(this.z));
            if (n2 > 0) {
                BlockSoundGroup blockSoundGroup = Block.BLOCKS[n2].soundGroup;
                this.world.playSound(this, blockSoundGroup.getSound(), blockSoundGroup.getVolume() * 0.5f, blockSoundGroup.getPitch() * 0.75f);
            }
        }
    }

    public void travel(float x, float z) {
        if (this.isSubmergedInWater()) {
            double d = this.y;
            this.moveNonSolid(x, z, 0.02f);
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            this.velocityX *= (double)0.8f;
            this.velocityY *= (double)0.8f;
            this.velocityZ *= (double)0.8f;
            this.velocityY -= 0.02;
            if (this.horizontalCollision && this.getEntitiesInside(this.velocityX, this.velocityY + (double)0.6f - this.y + d, this.velocityZ)) {
                this.velocityY = 0.3f;
            }
        } else if (this.isTouchingLava()) {
            double d = this.y;
            this.moveNonSolid(x, z, 0.02f);
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            this.velocityX *= 0.5;
            this.velocityY *= 0.5;
            this.velocityZ *= 0.5;
            this.velocityY -= 0.02;
            if (this.horizontalCollision && this.getEntitiesInside(this.velocityX, this.velocityY + (double)0.6f - this.y + d, this.velocityZ)) {
                this.velocityY = 0.3f;
            }
        } else {
            float f = 0.91f;
            if (this.onGround) {
                f = 0.54600006f;
                int n = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.boundingBox.minY) - 1, MathHelper.floor(this.z));
                if (n > 0) {
                    f = Block.BLOCKS[n].slipperiness * 0.91f;
                }
            }
            float f2 = 0.16277136f / (f * f * f);
            this.moveNonSolid(x, z, this.onGround ? 0.1f * f2 : 0.02f);
            f = 0.91f;
            if (this.onGround) {
                f = 0.54600006f;
                int n = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.boundingBox.minY) - 1, MathHelper.floor(this.z));
                if (n > 0) {
                    f = Block.BLOCKS[n].slipperiness * 0.91f;
                }
            }
            if (this.isOnLadder()) {
                float f3 = 0.15f;
                if (this.velocityX < (double)(-f3)) {
                    this.velocityX = -f3;
                }
                if (this.velocityX > (double)f3) {
                    this.velocityX = f3;
                }
                if (this.velocityZ < (double)(-f3)) {
                    this.velocityZ = -f3;
                }
                if (this.velocityZ > (double)f3) {
                    this.velocityZ = f3;
                }
                this.fallDistance = 0.0f;
                if (this.velocityY < -0.15) {
                    this.velocityY = -0.15;
                }
                if (this.isSneaking() && this.velocityY < 0.0) {
                    this.velocityY = 0.0;
                }
            }
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            if (this.horizontalCollision && this.isOnLadder()) {
                this.velocityY = 0.2;
            }
            this.velocityY -= 0.08;
            this.velocityY *= (double)0.98f;
            this.velocityX *= (double)f;
            this.velocityZ *= (double)f;
        }
        this.lastWalkAnimationSpeed = this.walkAnimationSpeed;
        double d = this.x - this.prevX;
        double d2 = this.z - this.prevZ;
        float f = MathHelper.sqrt(d * d + d2 * d2) * 4.0f;
        if (f > 1.0f) {
            f = 1.0f;
        }
        this.walkAnimationSpeed += (f - this.walkAnimationSpeed) * 0.4f;
        this.walkAnimationProgress += this.walkAnimationSpeed;
    }

    public boolean isOnLadder() {
        int n;
        int n2;
        int n3 = MathHelper.floor(this.x);
        return this.world.getBlockId(n3, n2 = MathHelper.floor(this.boundingBox.minY), n = MathHelper.floor(this.z)) == Block.LADDER.id;
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putShort("Health", (short)this.health);
        nbt.putShort("HurtTime", (short)this.hurtTime);
        nbt.putShort("DeathTime", (short)this.deathTime);
        nbt.putShort("AttackTime", (short)this.attackCooldown);
    }

    public void readNbt(NbtCompound nbt) {
        this.health = nbt.getShort("Health");
        if (!nbt.contains("Health")) {
            this.health = 10;
        }
        this.hurtTime = nbt.getShort("HurtTime");
        this.deathTime = nbt.getShort("DeathTime");
        this.attackCooldown = nbt.getShort("AttackTime");
    }

    public boolean isAlive() {
        return !this.dead && this.health > 0;
    }

    public boolean canBreatheInWater() {
        return false;
    }

    public void tickMovement() {
        if (this.bodyTrackingIncrements > 0) {
            double d;
            double d2 = this.x + (this.lerpX - this.x) / (double)this.bodyTrackingIncrements;
            double d3 = this.y + (this.lerpY - this.y) / (double)this.bodyTrackingIncrements;
            double d4 = this.z + (this.lerpZ - this.z) / (double)this.bodyTrackingIncrements;
            for (d = this.lerpYaw - (double)this.yaw; d < -180.0; d += 360.0) {
            }
            while (d >= 180.0) {
                d -= 360.0;
            }
            this.yaw = (float)((double)this.yaw + d / (double)this.bodyTrackingIncrements);
            this.pitch = (float)((double)this.pitch + (this.lerpPitch - (double)this.pitch) / (double)this.bodyTrackingIncrements);
            --this.bodyTrackingIncrements;
            this.setPosition(d2, d3, d4);
            this.setRotation(this.yaw, this.pitch);
            List list = this.world.getEntityCollisions(this, this.boundingBox.contract(0.03125, 0.0, 0.03125));
            if (list.size() > 0) {
                double d5 = 0.0;
                for (int i = 0; i < list.size(); ++i) {
                    Box box = (Box)list.get(i);
                    if (!(box.maxY > d5)) continue;
                    d5 = box.maxY;
                }
                this.setPosition(d2, d3 += d5 - this.boundingBox.minY, d4);
            }
        }
        if (this.isImmobile()) {
            this.jumping = false;
            this.sidewaysSpeed = 0.0f;
            this.forwardSpeed = 0.0f;
            this.rotationSpeed = 0.0f;
        } else if (!this.interpolateOnly) {
            this.tickLiving();
        }
        boolean bl = this.isSubmergedInWater();
        boolean bl2 = this.isTouchingLava();
        if (this.jumping) {
            if (bl) {
                this.velocityY += (double)0.04f;
            } else if (bl2) {
                this.velocityY += (double)0.04f;
            } else if (this.onGround) {
                this.jump();
            }
        }
        this.sidewaysSpeed *= 0.98f;
        this.forwardSpeed *= 0.98f;
        this.rotationSpeed *= 0.9f;
        this.travel(this.sidewaysSpeed, this.forwardSpeed);
        List list = this.world.getEntities(this, this.boundingBox.expand(0.2f, 0.0, 0.2f));
        if (list != null && list.size() > 0) {
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                if (!entity.isPushable()) continue;
                entity.onCollision(this);
            }
        }
    }

    protected boolean isImmobile() {
        return this.health <= 0;
    }

    protected void jump() {
        this.velocityY = 0.42f;
    }

    protected boolean canDespawn() {
        return true;
    }

    protected void tryDespawn() {
        PlayerEntity playerEntity = this.world.getClosestPlayer(this, -1.0);
        if (this.canDespawn() && playerEntity != null) {
            double d = playerEntity.x - this.x;
            double d2 = playerEntity.y - this.y;
            double d3 = playerEntity.z - this.z;
            double d4 = d * d + d2 * d2 + d3 * d3;
            if (d4 > 16384.0) {
                this.markDead();
            }
            if (this.despawnCounter > 600 && this.random.nextInt(800) == 0) {
                if (d4 < 1024.0) {
                    this.despawnCounter = 0;
                } else {
                    this.markDead();
                }
            }
        }
    }

    protected void tickLiving() {
        ++this.despawnCounter;
        PlayerEntity playerEntity = this.world.getClosestPlayer(this, -1.0);
        this.tryDespawn();
        this.sidewaysSpeed = 0.0f;
        this.forwardSpeed = 0.0f;
        float f = 8.0f;
        if (this.random.nextFloat() < 0.02f) {
            playerEntity = this.world.getClosestPlayer(this, f);
            if (playerEntity != null) {
                this.lookTarget = playerEntity;
                this.lookTimer = 10 + this.random.nextInt(20);
            } else {
                this.rotationSpeed = (this.random.nextFloat() - 0.5f) * 20.0f;
            }
        }
        if (this.lookTarget != null) {
            this.lookAt(this.lookTarget, 10.0f, this.getMaxLookPitchChange());
            if (this.lookTimer-- <= 0 || this.lookTarget.dead || this.lookTarget.getSquaredDistance(this) > (double)(f * f)) {
                this.lookTarget = null;
            }
        } else {
            if (this.random.nextFloat() < 0.05f) {
                this.rotationSpeed = (this.random.nextFloat() - 0.5f) * 20.0f;
            }
            this.yaw += this.rotationSpeed;
            this.pitch = this.defaultPitch;
        }
        boolean bl = this.isSubmergedInWater();
        boolean bl2 = this.isTouchingLava();
        if (bl || bl2) {
            this.jumping = this.random.nextFloat() < 0.8f;
        }
    }

    protected int getMaxLookPitchChange() {
        return 40;
    }

    public void lookAt(Entity target, float maxPitch, float maxYaw) {
        double d;
        double d2 = target.x - this.x;
        double d3 = target.z - this.z;
        if (target instanceof LivingEntity) {
            LivingEntity livingEntity = (LivingEntity)target;
            d = this.y + (double)this.getEyeHeight() - (livingEntity.y + (double)livingEntity.getEyeHeight());
        } else {
            d = (target.boundingBox.minY + target.boundingBox.maxY) / 2.0 - (this.y + (double)this.getEyeHeight());
        }
        double d4 = MathHelper.sqrt(d2 * d2 + d3 * d3);
        float f = (float)(Math.atan2(d3, d2) * 180.0 / 3.1415927410125732) - 90.0f;
        float f2 = (float)(-(Math.atan2(d, d4) * 180.0 / 3.1415927410125732));
        this.pitch = -this.lerpRotation(this.pitch, f2, maxYaw);
        this.yaw = this.lerpRotation(this.yaw, f, maxPitch);
    }

    public boolean hasLookTarget() {
        return this.lookTarget != null;
    }

    public Entity getLookTarget() {
        return this.lookTarget;
    }

    private float lerpRotation(float from, float to, float maxChange) {
        float f;
        for (f = to - from; f < -180.0f; f += 360.0f) {
        }
        while (f >= 180.0f) {
            f -= 360.0f;
        }
        if (f > maxChange) {
            f = maxChange;
        }
        if (f < -maxChange) {
            f = -maxChange;
        }
        return from + f;
    }

    public void beforeRemove() {
    }

    public boolean canSpawn() {
        return this.world.canSpawnEntity(this.boundingBox) && this.world.getEntityCollisions(this, this.boundingBox).size() == 0 && !this.world.isBoxSubmergedInFluid(this.boundingBox);
    }

    protected void tickInVoid() {
        this.damage(null, 4);
    }

    @Environment(value=EnvType.CLIENT)
    public float getHandSwingProgress(float f) {
        float f2 = this.swingAnimationProgress - this.lastSwingAnimationProgress;
        if (f2 < 0.0f) {
            f2 += 1.0f;
        }
        return this.lastSwingAnimationProgress + f2 * f;
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getPosition(float tickDelta) {
        if (tickDelta == 1.0f) {
            return Vec3d.createCached(this.x, this.y, this.z);
        }
        double d = this.prevX + (this.x - this.prevX) * (double)tickDelta;
        double d2 = this.prevY + (this.y - this.prevY) * (double)tickDelta;
        double d3 = this.prevZ + (this.z - this.prevZ) * (double)tickDelta;
        return Vec3d.createCached(d, d2, d3);
    }

    public Vec3d getLookVector() {
        return this.getLookVector(1.0f);
    }

    public Vec3d getLookVector(float tickDelta) {
        if (tickDelta == 1.0f) {
            float f = MathHelper.cos(-this.yaw * ((float)Math.PI / 180) - (float)Math.PI);
            float f2 = MathHelper.sin(-this.yaw * ((float)Math.PI / 180) - (float)Math.PI);
            float f3 = -MathHelper.cos(-this.pitch * ((float)Math.PI / 180));
            float f4 = MathHelper.sin(-this.pitch * ((float)Math.PI / 180));
            return Vec3d.createCached(f2 * f3, f4, f * f3);
        }
        float f = this.prevPitch + (this.pitch - this.prevPitch) * tickDelta;
        float f5 = this.prevYaw + (this.yaw - this.prevYaw) * tickDelta;
        float f6 = MathHelper.cos(-f5 * ((float)Math.PI / 180) - (float)Math.PI);
        float f7 = MathHelper.sin(-f5 * ((float)Math.PI / 180) - (float)Math.PI);
        float f8 = -MathHelper.cos(-f * ((float)Math.PI / 180));
        float f9 = MathHelper.sin(-f * ((float)Math.PI / 180));
        return Vec3d.createCached(f7 * f8, f9, f6 * f8);
    }

    @Environment(value=EnvType.CLIENT)
    public HitResult raycast(double distance, float tickDelta) {
        Vec3d vec3d = this.getPosition(tickDelta);
        Vec3d vec3d2 = this.getLookVector(tickDelta);
        Vec3d vec3d3 = vec3d.add(vec3d2.x * distance, vec3d2.y * distance, vec3d2.z * distance);
        return this.world.raycast(vec3d, vec3d3);
    }

    public int getLimitPerChunk() {
        return 4;
    }

    @Environment(value=EnvType.CLIENT)
    public ItemStack getHeldItem() {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public void processServerEntityStatus(byte status) {
        if (status == 2) {
            this.walkAnimationSpeed = 1.5f;
            this.hearts = this.maxHealth;
            this.damagedTime = 10;
            this.hurtTime = 10;
            this.damagedSwingDir = 0.0f;
            this.world.playSound(this, this.getHurtSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
            this.damage(null, 0);
        } else if (status == 3) {
            this.world.playSound(this, this.getDeathSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
            this.health = 0;
            this.onKilledBy(null);
        } else {
            super.processServerEntityStatus(status);
        }
    }

    public boolean isSleeping() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getItemStackTextureId(ItemStack stack) {
        return stack.getTextureId();
    }
}

