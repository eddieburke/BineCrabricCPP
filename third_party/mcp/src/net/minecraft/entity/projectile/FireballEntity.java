/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.projectile;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class FireballEntity
extends Entity {
    private int blockX = -1;
    private int blockY = -1;
    private int blockZ = -1;
    private int blockId = 0;
    private boolean inGround = false;
    public int shake = 0;
    public LivingEntity owner;
    private int removalTimer;
    private int inAirTime = 0;
    public double powerX;
    public double powerY;
    public double powerZ;

    public FireballEntity(World arg) {
        super(arg);
        this.setBoundingBoxSpacing(1.0f, 1.0f);
    }

    protected void initDataTracker() {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(double distance) {
        double d = this.boundingBox.getAverageSideLength() * 4.0;
        return distance < (d *= 64.0) * d;
    }

    @Environment(value=EnvType.CLIENT)
    public FireballEntity(World world, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
        super(world);
        this.setBoundingBoxSpacing(1.0f, 1.0f);
        this.setPositionAndAnglesKeepPrevAngles(x, y, z, this.yaw, this.pitch);
        this.setPosition(x, y, z);
        double d = MathHelper.sqrt(velocityX * velocityX + velocityY * velocityY + velocityZ * velocityZ);
        this.powerX = velocityX / d * 0.1;
        this.powerY = velocityY / d * 0.1;
        this.powerZ = velocityZ / d * 0.1;
    }

    public FireballEntity(World world, LivingEntity owner, double velocityX, double velocityY, double velocityZ) {
        super(world);
        this.owner = owner;
        this.setBoundingBoxSpacing(1.0f, 1.0f);
        this.setPositionAndAnglesKeepPrevAngles(owner.x, owner.y, owner.z, owner.yaw, owner.pitch);
        this.setPosition(this.x, this.y, this.z);
        this.standingEyeHeight = 0.0f;
        this.velocityZ = 0.0;
        this.velocityY = 0.0;
        this.velocityX = 0.0;
        double d = MathHelper.sqrt((velocityX += this.random.nextGaussian() * 0.4) * velocityX + (velocityY += this.random.nextGaussian() * 0.4) * velocityY + (velocityZ += this.random.nextGaussian() * 0.4) * velocityZ);
        this.powerX = velocityX / d * 0.1;
        this.powerY = velocityY / d * 0.1;
        this.powerZ = velocityZ / d * 0.1;
    }

    /*
     * Enabled aggressive block sorting
     */
    public void tick() {
        block17: {
            super.tick();
            this.fireTicks = 10;
            if (this.shake > 0) {
                --this.shake;
            }
            if (this.inGround) {
                int n = this.world.getBlockId(this.blockX, this.blockY, this.blockZ);
                if (n != this.blockId) {
                    this.inGround = false;
                    this.velocityX *= (double)(this.random.nextFloat() * 0.2f);
                    this.velocityY *= (double)(this.random.nextFloat() * 0.2f);
                    this.velocityZ *= (double)(this.random.nextFloat() * 0.2f);
                    this.removalTimer = 0;
                    this.inAirTime = 0;
                    break block17;
                } else {
                    ++this.removalTimer;
                    if (this.removalTimer == 1200) {
                        this.markDead();
                    }
                    return;
                }
            }
            ++this.inAirTime;
        }
        Vec3d vec3d = Vec3d.createCached(this.x, this.y, this.z);
        Vec3d vec3d2 = Vec3d.createCached(this.x + this.velocityX, this.y + this.velocityY, this.z + this.velocityZ);
        HitResult hitResult = this.world.raycast(vec3d, vec3d2);
        vec3d = Vec3d.createCached(this.x, this.y, this.z);
        vec3d2 = Vec3d.createCached(this.x + this.velocityX, this.y + this.velocityY, this.z + this.velocityZ);
        if (hitResult != null) {
            vec3d2 = Vec3d.createCached(hitResult.pos.x, hitResult.pos.y, hitResult.pos.z);
        }
        Entity entity = null;
        List list = this.world.getEntities(this, this.boundingBox.stretch(this.velocityX, this.velocityY, this.velocityZ).expand(1.0, 1.0, 1.0));
        double d = 0.0;
        for (int i = 0; i < list.size(); ++i) {
            double d2;
            float f;
            Box box;
            HitResult hitResult2;
            Entity entity2 = (Entity)list.get(i);
            if (!entity2.isCollidable() || entity2 == this.owner && this.inAirTime < 25 || (hitResult2 = (box = entity2.boundingBox.expand(f = 0.3f, f, f)).raycast(vec3d, vec3d2)) == null || !((d2 = vec3d.distanceTo(hitResult2.pos)) < d) && d != 0.0) continue;
            entity = entity2;
            d = d2;
        }
        if (entity != null) {
            hitResult = new HitResult(entity);
        }
        if (hitResult != null) {
            if (!this.world.isRemote) {
                if (hitResult.entity == null || hitResult.entity.damage(this.owner, 0)) {
                    // empty if block
                }
                this.world.createExplosion(null, this.x, this.y, this.z, 1.0f, true);
            }
            this.markDead();
        }
        this.x += this.velocityX;
        this.y += this.velocityY;
        this.z += this.velocityZ;
        float f = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
        this.yaw = (float)(Math.atan2(this.velocityX, this.velocityZ) * 180.0 / 3.1415927410125732);
        this.pitch = (float)(Math.atan2(this.velocityY, f) * 180.0 / 3.1415927410125732);
        while (this.pitch - this.prevPitch < -180.0f) {
            this.prevPitch -= 360.0f;
        }
        while (this.pitch - this.prevPitch >= 180.0f) {
            this.prevPitch += 360.0f;
        }
        while (this.yaw - this.prevYaw < -180.0f) {
            this.prevYaw -= 360.0f;
        }
        while (this.yaw - this.prevYaw >= 180.0f) {
            this.prevYaw += 360.0f;
        }
        this.pitch = this.prevPitch + (this.pitch - this.prevPitch) * 0.2f;
        this.yaw = this.prevYaw + (this.yaw - this.prevYaw) * 0.2f;
        float f2 = 0.95f;
        if (this.isSubmergedInWater()) {
            for (int i = 0; i < 4; ++i) {
                float f3 = 0.25f;
                this.world.addParticle("bubble", this.x - this.velocityX * (double)f3, this.y - this.velocityY * (double)f3, this.z - this.velocityZ * (double)f3, this.velocityX, this.velocityY, this.velocityZ);
            }
            f2 = 0.8f;
        }
        this.velocityX += this.powerX;
        this.velocityY += this.powerY;
        this.velocityZ += this.powerZ;
        this.velocityX *= (double)f2;
        this.velocityY *= (double)f2;
        this.velocityZ *= (double)f2;
        this.world.addParticle("smoke", this.x, this.y + 0.5, this.z, 0.0, 0.0, 0.0);
        this.setPosition(this.x, this.y, this.z);
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putShort("xTile", (short)this.blockX);
        nbt.putShort("yTile", (short)this.blockY);
        nbt.putShort("zTile", (short)this.blockZ);
        nbt.putByte("inTile", (byte)this.blockId);
        nbt.putByte("shake", (byte)this.shake);
        nbt.putByte("inGround", (byte)(this.inGround ? 1 : 0));
    }

    public void readNbt(NbtCompound nbt) {
        this.blockX = nbt.getShort("xTile");
        this.blockY = nbt.getShort("yTile");
        this.blockZ = nbt.getShort("zTile");
        this.blockId = nbt.getByte("inTile") & 0xFF;
        this.shake = nbt.getByte("shake") & 0xFF;
        this.inGround = nbt.getByte("inGround") == 1;
    }

    public boolean isCollidable() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public float getTargetingMargin() {
        return 1.0f;
    }

    public boolean damage(Entity damageSource, int amount) {
        this.scheduleVelocityUpdate();
        if (damageSource != null) {
            Vec3d vec3d = damageSource.getLookVector();
            if (vec3d != null) {
                this.velocityX = vec3d.x;
                this.velocityY = vec3d.y;
                this.velocityZ = vec3d.z;
                this.powerX = this.velocityX * 0.1;
                this.powerY = this.velocityY * 0.1;
                this.powerZ = this.velocityZ * 0.1;
            }
            return true;
        }
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }
}

