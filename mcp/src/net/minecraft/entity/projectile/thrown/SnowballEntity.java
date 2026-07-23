/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.projectile.thrown;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class SnowballEntity
extends Entity {
    private int blockX = -1;
    private int blockY = -1;
    private int blockZ = -1;
    private int blockId = 0;
    private boolean inGround = false;
    public int shake = 0;
    private LivingEntity owner;
    private int removalTimer;
    private int inAirTime = 0;

    public SnowballEntity(World arg) {
        super(arg);
        this.setBoundingBoxSpacing(0.25f, 0.25f);
    }

    protected void initDataTracker() {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(double distance) {
        double d = this.boundingBox.getAverageSideLength() * 4.0;
        return distance < (d *= 64.0) * d;
    }

    public SnowballEntity(World world, LivingEntity owner) {
        super(world);
        this.owner = owner;
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.setPositionAndAnglesKeepPrevAngles(owner.x, owner.y + (double)owner.getEyeHeight(), owner.z, owner.yaw, owner.pitch);
        this.x -= (double)(MathHelper.cos(this.yaw / 180.0f * (float)Math.PI) * 0.16f);
        this.y -= (double)0.1f;
        this.z -= (double)(MathHelper.sin(this.yaw / 180.0f * (float)Math.PI) * 0.16f);
        this.setPosition(this.x, this.y, this.z);
        this.standingEyeHeight = 0.0f;
        float f = 0.4f;
        this.velocityX = -MathHelper.sin(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI) * f;
        this.velocityZ = MathHelper.cos(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI) * f;
        this.velocityY = -MathHelper.sin(this.pitch / 180.0f * (float)Math.PI) * f;
        this.setVelocity(this.velocityX, this.velocityY, this.velocityZ, 1.5f, 1.0f);
    }

    public SnowballEntity(World world, double x, double y, double z) {
        super(world);
        this.removalTimer = 0;
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.setPosition(x, y, z);
        this.standingEyeHeight = 0.0f;
    }

    public void setVelocity(double x, double y, double z, float speed, float divergence) {
        float f = MathHelper.sqrt(x * x + y * y + z * z);
        x /= (double)f;
        y /= (double)f;
        z /= (double)f;
        x += this.random.nextGaussian() * (double)0.0075f * (double)divergence;
        y += this.random.nextGaussian() * (double)0.0075f * (double)divergence;
        z += this.random.nextGaussian() * (double)0.0075f * (double)divergence;
        this.velocityX = x *= (double)speed;
        this.velocityY = y *= (double)speed;
        this.velocityZ = z *= (double)speed;
        float f2 = MathHelper.sqrt(x * x + z * z);
        this.prevYaw = this.yaw = (float)(Math.atan2(x, z) * 180.0 / 3.1415927410125732);
        this.prevPitch = this.pitch = (float)(Math.atan2(y, f2) * 180.0 / 3.1415927410125732);
        this.removalTimer = 0;
    }

    @Environment(value=EnvType.CLIENT)
    public void setVelocityClient(double x, double y, double z) {
        this.velocityX = x;
        this.velocityY = y;
        this.velocityZ = z;
        if (this.prevPitch == 0.0f && this.prevYaw == 0.0f) {
            float f = MathHelper.sqrt(x * x + z * z);
            this.prevYaw = this.yaw = (float)(Math.atan2(x, z) * 180.0 / 3.1415927410125732);
            this.prevPitch = this.pitch = (float)(Math.atan2(y, f) * 180.0 / 3.1415927410125732);
        }
    }

    /*
     * Enabled aggressive block sorting
     */
    public void tick() {
        block18: {
            this.lastTickX = this.x;
            this.lastTickY = this.y;
            this.lastTickZ = this.z;
            super.tick();
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
                    break block18;
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
        if (!this.world.isRemote) {
            Entity entity = null;
            List list = this.world.getEntities(this, this.boundingBox.stretch(this.velocityX, this.velocityY, this.velocityZ).expand(1.0, 1.0, 1.0));
            double d = 0.0;
            for (int i = 0; i < list.size(); ++i) {
                double d2;
                float f;
                Box box;
                HitResult hitResult2;
                Entity entity2 = (Entity)list.get(i);
                if (!entity2.isCollidable() || entity2 == this.owner && this.inAirTime < 5 || (hitResult2 = (box = entity2.boundingBox.expand(f = 0.3f, f, f)).raycast(vec3d, vec3d2)) == null || !((d2 = vec3d.distanceTo(hitResult2.pos)) < d) && d != 0.0) continue;
                entity = entity2;
                d = d2;
            }
            if (entity != null) {
                hitResult = new HitResult(entity);
            }
        }
        if (hitResult != null) {
            if (hitResult.entity == null || hitResult.entity.damage(this.owner, 0)) {
                // empty if block
            }
            for (int i = 0; i < 8; ++i) {
                this.world.addParticle("snowballpoof", this.x, this.y, this.z, 0.0, 0.0, 0.0);
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
        float f2 = 0.99f;
        float f3 = 0.03f;
        if (this.isSubmergedInWater()) {
            for (int i = 0; i < 4; ++i) {
                float f4 = 0.25f;
                this.world.addParticle("bubble", this.x - this.velocityX * (double)f4, this.y - this.velocityY * (double)f4, this.z - this.velocityZ * (double)f4, this.velocityX, this.velocityY, this.velocityZ);
            }
            f2 = 0.8f;
        }
        this.velocityX *= (double)f2;
        this.velocityY *= (double)f2;
        this.velocityZ *= (double)f2;
        this.velocityY -= (double)f3;
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

    public void onPlayerInteraction(PlayerEntity player) {
        if (this.inGround && this.owner == player && this.shake <= 0 && player.inventory.addStack(new ItemStack(Item.ARROW, 1))) {
            this.world.playSound(this, "random.pop", 0.2f, ((this.random.nextFloat() - this.random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
            player.sendPickup(this, 1);
            this.markDead();
        }
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }
}

