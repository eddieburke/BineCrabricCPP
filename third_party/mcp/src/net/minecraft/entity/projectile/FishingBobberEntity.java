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
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.stat.Stats;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class FishingBobberEntity
extends Entity {
    private int blockX = -1;
    private int blockY = -1;
    private int blockZ = -1;
    private int blockId = 0;
    private boolean inGround = false;
    public int shake = 0;
    public PlayerEntity owner;
    private int removalTimer;
    private int inAirTime = 0;
    private int hookCountdown = 0;
    public Entity hookedEntity = null;
    private int clientInterpolationSteps;
    private double clientX;
    private double clientY;
    private double clientZ;
    private double clientPitch;
    private double clientYaw;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityX;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityY;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityZ;

    public FishingBobberEntity(World arg) {
        super(arg);
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.ignoreFrustumCull = true;
    }

    @Environment(value=EnvType.CLIENT)
    public FishingBobberEntity(World world, double x, double y, double z) {
        this(world);
        this.setPosition(x, y, z);
        this.ignoreFrustumCull = true;
    }

    public FishingBobberEntity(World world, PlayerEntity thrower) {
        super(world);
        this.ignoreFrustumCull = true;
        this.owner = thrower;
        this.owner.fishHook = this;
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.setPositionAndAnglesKeepPrevAngles(thrower.x, thrower.y + 1.62 - (double)thrower.standingEyeHeight, thrower.z, thrower.yaw, thrower.pitch);
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

    protected void initDataTracker() {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(double distance) {
        double d = this.boundingBox.getAverageSideLength() * 4.0;
        return distance < (d *= 64.0) * d;
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
    public void setPositionAndAnglesAvoidEntities(double x, double y, double z, float pitch, float yaw, int interpolationSteps) {
        this.clientX = x;
        this.clientY = y;
        this.clientZ = z;
        this.clientPitch = pitch;
        this.clientYaw = yaw;
        this.clientInterpolationSteps = interpolationSteps;
        this.velocityX = this.clientVelocityX;
        this.velocityY = this.clientVelocityY;
        this.velocityZ = this.clientVelocityZ;
    }

    @Environment(value=EnvType.CLIENT)
    public void setVelocityClient(double x, double y, double z) {
        this.clientVelocityX = this.velocityX = x;
        this.clientVelocityY = this.velocityY = y;
        this.clientVelocityZ = this.velocityZ = z;
    }

    /*
     * Enabled aggressive block sorting
     */
    public void tick() {
        int n;
        block36: {
            super.tick();
            if (this.clientInterpolationSteps > 0) {
                double d;
                double d2 = this.x + (this.clientX - this.x) / (double)this.clientInterpolationSteps;
                double d3 = this.y + (this.clientY - this.y) / (double)this.clientInterpolationSteps;
                double d4 = this.z + (this.clientZ - this.z) / (double)this.clientInterpolationSteps;
                for (d = this.clientPitch - (double)this.yaw; d < -180.0; d += 360.0) {
                }
                while (true) {
                    if (!(d >= 180.0)) {
                        this.yaw = (float)((double)this.yaw + d / (double)this.clientInterpolationSteps);
                        this.pitch = (float)((double)this.pitch + (this.clientYaw - (double)this.pitch) / (double)this.clientInterpolationSteps);
                        --this.clientInterpolationSteps;
                        this.setPosition(d2, d3, d4);
                        this.setRotation(this.yaw, this.pitch);
                        return;
                    }
                    d -= 360.0;
                }
            }
            if (!this.world.isRemote) {
                ItemStack itemStack = this.owner.getHand();
                if (this.owner.dead || !this.owner.isAlive() || itemStack == null || itemStack.getItem() != Item.FISHING_ROD || this.getSquaredDistance(this.owner) > 1024.0) {
                    this.markDead();
                    this.owner.fishHook = null;
                    return;
                }
                if (this.hookedEntity != null) {
                    if (!this.hookedEntity.dead) {
                        this.x = this.hookedEntity.x;
                        this.y = this.hookedEntity.boundingBox.minY + (double)this.hookedEntity.height * 0.8;
                        this.z = this.hookedEntity.z;
                        return;
                    }
                    this.hookedEntity = null;
                }
            }
            if (this.shake > 0) {
                --this.shake;
            }
            if (this.inGround) {
                int n2 = this.world.getBlockId(this.blockX, this.blockY, this.blockZ);
                if (n2 != this.blockId) {
                    this.inGround = false;
                    this.velocityX *= (double)(this.random.nextFloat() * 0.2f);
                    this.velocityY *= (double)(this.random.nextFloat() * 0.2f);
                    this.velocityZ *= (double)(this.random.nextFloat() * 0.2f);
                    this.removalTimer = 0;
                    this.inAirTime = 0;
                    break block36;
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
            double d5;
            float f;
            Box box;
            HitResult hitResult2;
            Entity entity2 = (Entity)list.get(i);
            if (!entity2.isCollidable() || entity2 == this.owner && this.inAirTime < 5 || (hitResult2 = (box = entity2.boundingBox.expand(f = 0.3f, f, f)).raycast(vec3d, vec3d2)) == null || !((d5 = vec3d.distanceTo(hitResult2.pos)) < d) && d != 0.0) continue;
            entity = entity2;
            d = d5;
        }
        if (entity != null) {
            hitResult = new HitResult(entity);
        }
        if (hitResult != null) {
            if (hitResult.entity != null) {
                if (hitResult.entity.damage(this.owner, 0)) {
                    this.hookedEntity = hitResult.entity;
                }
            } else {
                this.inGround = true;
            }
        }
        if (this.inGround) {
            return;
        }
        this.move(this.velocityX, this.velocityY, this.velocityZ);
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
        float f2 = 0.92f;
        if (this.onGround || this.horizontalCollision) {
            f2 = 0.5f;
        }
        int n3 = 5;
        double d6 = 0.0;
        for (n = 0; n < n3; ++n) {
            double d7 = this.boundingBox.minY + (this.boundingBox.maxY - this.boundingBox.minY) * (double)(n + 0) / (double)n3 - 0.125 + 0.125;
            double d8 = this.boundingBox.minY + (this.boundingBox.maxY - this.boundingBox.minY) * (double)(n + 1) / (double)n3 - 0.125 + 0.125;
            Box box = Box.createCached(this.boundingBox.minX, d7, this.boundingBox.minZ, this.boundingBox.maxX, d8, this.boundingBox.maxZ);
            if (!this.world.isFluidInBox(box, Material.WATER)) continue;
            d6 += 1.0 / (double)n3;
        }
        if (d6 > 0.0) {
            if (this.hookCountdown > 0) {
                --this.hookCountdown;
            } else {
                n = 500;
                if (this.world.isRaining(MathHelper.floor(this.x), MathHelper.floor(this.y) + 1, MathHelper.floor(this.z))) {
                    n = 300;
                }
                if (this.random.nextInt(n) == 0) {
                    float f3;
                    this.hookCountdown = this.random.nextInt(30) + 10;
                    this.velocityY -= (double)0.2f;
                    this.world.playSound(this, "random.splash", 0.25f, 1.0f + (this.random.nextFloat() - this.random.nextFloat()) * 0.4f);
                    float f4 = MathHelper.floor(this.boundingBox.minY);
                    int n4 = 0;
                    while ((float)n4 < 1.0f + this.width * 20.0f) {
                        float f5 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                        f3 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                        this.world.addParticle("bubble", this.x + (double)f5, f4 + 1.0f, this.z + (double)f3, this.velocityX, this.velocityY - (double)(this.random.nextFloat() * 0.2f), this.velocityZ);
                        ++n4;
                    }
                    n4 = 0;
                    while ((float)n4 < 1.0f + this.width * 20.0f) {
                        float f6 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                        f3 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                        this.world.addParticle("splash", this.x + (double)f6, f4 + 1.0f, this.z + (double)f3, this.velocityX, this.velocityY, this.velocityZ);
                        ++n4;
                    }
                }
            }
        }
        if (this.hookCountdown > 0) {
            this.velocityY -= (double)(this.random.nextFloat() * this.random.nextFloat() * this.random.nextFloat()) * 0.2;
        }
        double d9 = d6 * 2.0 - 1.0;
        this.velocityY += (double)0.04f * d9;
        if (d6 > 0.0) {
            f2 = (float)((double)f2 * 0.9);
            this.velocityY *= 0.8;
        }
        this.velocityX *= (double)f2;
        this.velocityY *= (double)f2;
        this.velocityZ *= (double)f2;
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

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }

    public int use() {
        int n = 0;
        if (this.hookedEntity != null) {
            double d = this.owner.x - this.x;
            double d2 = this.owner.y - this.y;
            double d3 = this.owner.z - this.z;
            double d4 = MathHelper.sqrt(d * d + d2 * d2 + d3 * d3);
            double d5 = 0.1;
            this.hookedEntity.velocityX += d * d5;
            this.hookedEntity.velocityY += d2 * d5 + (double)MathHelper.sqrt(d4) * 0.08;
            this.hookedEntity.velocityZ += d3 * d5;
            n = 3;
        } else if (this.hookCountdown > 0) {
            ItemEntity itemEntity = new ItemEntity(this.world, this.x, this.y, this.z, new ItemStack(Item.RAW_FISH));
            double d = this.owner.x - this.x;
            double d6 = this.owner.y - this.y;
            double d7 = this.owner.z - this.z;
            double d8 = MathHelper.sqrt(d * d + d6 * d6 + d7 * d7);
            double d9 = 0.1;
            itemEntity.velocityX = d * d9;
            itemEntity.velocityY = d6 * d9 + (double)MathHelper.sqrt(d8) * 0.08;
            itemEntity.velocityZ = d7 * d9;
            this.world.spawnEntity(itemEntity);
            this.owner.increaseStat(Stats.FISH_CAUGHT, 1);
            n = 1;
        }
        if (this.inGround) {
            n = 2;
        }
        this.markDead();
        this.owner.fishHook = null;
        return n;
    }
}

