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
import net.minecraft.block.Block;
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

public class ArrowEntity
extends Entity {
    private int blockX = -1;
    private int blockY = -1;
    private int blockZ = -1;
    private int blockId = 0;
    private int blockMeta = 0;
    private boolean inGround = false;
    public boolean pickupAllowed = false;
    public int shake = 0;
    public LivingEntity owner;
    private int life;
    private int inAirTime = 0;

    public ArrowEntity(World arg) {
        super(arg);
        this.setBoundingBoxSpacing(0.5f, 0.5f);
    }

    public ArrowEntity(World world, double x, double y, double z) {
        super(world);
        this.setBoundingBoxSpacing(0.5f, 0.5f);
        this.setPosition(x, y, z);
        this.standingEyeHeight = 0.0f;
    }

    public ArrowEntity(World world, LivingEntity owner) {
        super(world);
        this.owner = owner;
        this.pickupAllowed = owner instanceof PlayerEntity;
        this.setBoundingBoxSpacing(0.5f, 0.5f);
        this.setPositionAndAnglesKeepPrevAngles(owner.x, owner.y + (double)owner.getEyeHeight(), owner.z, owner.yaw, owner.pitch);
        this.x -= (double)(MathHelper.cos(this.yaw / 180.0f * (float)Math.PI) * 0.16f);
        this.y -= (double)0.1f;
        this.z -= (double)(MathHelper.sin(this.yaw / 180.0f * (float)Math.PI) * 0.16f);
        this.setPosition(this.x, this.y, this.z);
        this.standingEyeHeight = 0.0f;
        this.velocityX = -MathHelper.sin(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI);
        this.velocityZ = MathHelper.cos(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI);
        this.velocityY = -MathHelper.sin(this.pitch / 180.0f * (float)Math.PI);
        this.setVelocity(this.velocityX, this.velocityY, this.velocityZ, 1.5f, 1.0f);
    }

    protected void initDataTracker() {
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
        this.life = 0;
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
            this.prevPitch = this.pitch;
            this.prevYaw = this.yaw;
            this.setPositionAndAnglesKeepPrevAngles(this.x, this.y, this.z, this.yaw, this.pitch);
            this.life = 0;
        }
    }

    public void tick() {
        float f;
        Object object;
        int n;
        super.tick();
        if (this.prevPitch == 0.0f && this.prevYaw == 0.0f) {
            float f2 = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
            this.prevYaw = this.yaw = (float)(Math.atan2(this.velocityX, this.velocityZ) * 180.0 / 3.1415927410125732);
            this.prevPitch = this.pitch = (float)(Math.atan2(this.velocityY, f2) * 180.0 / 3.1415927410125732);
        }
        if ((n = this.world.getBlockId(this.blockX, this.blockY, this.blockZ)) > 0) {
            Block.BLOCKS[n].updateBoundingBox(this.world, this.blockX, this.blockY, this.blockZ);
            object = Block.BLOCKS[n].getCollisionShape(this.world, this.blockX, this.blockY, this.blockZ);
            if (object != null && ((Box)object).contains(Vec3d.createCached(this.x, this.y, this.z))) {
                this.inGround = true;
            }
        }
        if (this.shake > 0) {
            --this.shake;
        }
        if (this.inGround) {
            n = this.world.getBlockId(this.blockX, this.blockY, this.blockZ);
            int n2 = this.world.getBlockMeta(this.blockX, this.blockY, this.blockZ);
            if (n != this.blockId || n2 != this.blockMeta) {
                this.inGround = false;
                this.velocityX *= (double)(this.random.nextFloat() * 0.2f);
                this.velocityY *= (double)(this.random.nextFloat() * 0.2f);
                this.velocityZ *= (double)(this.random.nextFloat() * 0.2f);
                this.life = 0;
                this.inAirTime = 0;
                return;
            }
            ++this.life;
            if (this.life == 1200) {
                this.markDead();
            }
            return;
        }
        ++this.inAirTime;
        Vec3d vec3d = Vec3d.createCached(this.x, this.y, this.z);
        object = Vec3d.createCached(this.x + this.velocityX, this.y + this.velocityY, this.z + this.velocityZ);
        HitResult hitResult = this.world.raycast(vec3d, (Vec3d)object, false, true);
        vec3d = Vec3d.createCached(this.x, this.y, this.z);
        object = Vec3d.createCached(this.x + this.velocityX, this.y + this.velocityY, this.z + this.velocityZ);
        if (hitResult != null) {
            object = Vec3d.createCached(hitResult.pos.x, hitResult.pos.y, hitResult.pos.z);
        }
        Entity entity = null;
        List list = this.world.getEntities(this, this.boundingBox.stretch(this.velocityX, this.velocityY, this.velocityZ).expand(1.0, 1.0, 1.0));
        double d = 0.0;
        for (int i = 0; i < list.size(); ++i) {
            double d2;
            Box box;
            HitResult hitResult2;
            Entity entity2 = (Entity)list.get(i);
            if (!entity2.isCollidable() || entity2 == this.owner && this.inAirTime < 5 || (hitResult2 = (box = entity2.boundingBox.expand(f = 0.3f, f, f)).raycast(vec3d, (Vec3d)object)) == null || !((d2 = vec3d.distanceTo(hitResult2.pos)) < d) && d != 0.0) continue;
            entity = entity2;
            d = d2;
        }
        if (entity != null) {
            hitResult = new HitResult(entity);
        }
        if (hitResult != null) {
            if (hitResult.entity != null) {
                if (hitResult.entity.damage(this.owner, 4)) {
                    this.world.playSound(this, "random.drr", 1.0f, 1.2f / (this.random.nextFloat() * 0.2f + 0.9f));
                    this.markDead();
                } else {
                    this.velocityX *= (double)-0.1f;
                    this.velocityY *= (double)-0.1f;
                    this.velocityZ *= (double)-0.1f;
                    this.yaw += 180.0f;
                    this.prevYaw += 180.0f;
                    this.inAirTime = 0;
                }
            } else {
                this.blockX = hitResult.blockX;
                this.blockY = hitResult.blockY;
                this.blockZ = hitResult.blockZ;
                this.blockId = this.world.getBlockId(this.blockX, this.blockY, this.blockZ);
                this.blockMeta = this.world.getBlockMeta(this.blockX, this.blockY, this.blockZ);
                this.velocityX = (float)(hitResult.pos.x - this.x);
                this.velocityY = (float)(hitResult.pos.y - this.y);
                this.velocityZ = (float)(hitResult.pos.z - this.z);
                float f3 = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityY * this.velocityY + this.velocityZ * this.velocityZ);
                this.x -= this.velocityX / (double)f3 * (double)0.05f;
                this.y -= this.velocityY / (double)f3 * (double)0.05f;
                this.z -= this.velocityZ / (double)f3 * (double)0.05f;
                this.world.playSound(this, "random.drr", 1.0f, 1.2f / (this.random.nextFloat() * 0.2f + 0.9f));
                this.inGround = true;
                this.shake = 7;
            }
        }
        this.x += this.velocityX;
        this.y += this.velocityY;
        this.z += this.velocityZ;
        float f4 = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
        this.yaw = (float)(Math.atan2(this.velocityX, this.velocityZ) * 180.0 / 3.1415927410125732);
        this.pitch = (float)(Math.atan2(this.velocityY, f4) * 180.0 / 3.1415927410125732);
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
        float f5 = 0.99f;
        f = 0.03f;
        if (this.isSubmergedInWater()) {
            for (int i = 0; i < 4; ++i) {
                float f6 = 0.25f;
                this.world.addParticle("bubble", this.x - this.velocityX * (double)f6, this.y - this.velocityY * (double)f6, this.z - this.velocityZ * (double)f6, this.velocityX, this.velocityY, this.velocityZ);
            }
            f5 = 0.8f;
        }
        this.velocityX *= (double)f5;
        this.velocityY *= (double)f5;
        this.velocityZ *= (double)f5;
        this.velocityY -= (double)f;
        this.setPosition(this.x, this.y, this.z);
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putShort("xTile", (short)this.blockX);
        nbt.putShort("yTile", (short)this.blockY);
        nbt.putShort("zTile", (short)this.blockZ);
        nbt.putByte("inTile", (byte)this.blockId);
        nbt.putByte("inData", (byte)this.blockMeta);
        nbt.putByte("shake", (byte)this.shake);
        nbt.putByte("inGround", (byte)(this.inGround ? 1 : 0));
        nbt.putBoolean("player", this.pickupAllowed);
    }

    public void readNbt(NbtCompound nbt) {
        this.blockX = nbt.getShort("xTile");
        this.blockY = nbt.getShort("yTile");
        this.blockZ = nbt.getShort("zTile");
        this.blockId = nbt.getByte("inTile") & 0xFF;
        this.blockMeta = nbt.getByte("inData") & 0xFF;
        this.shake = nbt.getByte("shake") & 0xFF;
        this.inGround = nbt.getByte("inGround") == 1;
        this.pickupAllowed = nbt.getBoolean("player");
    }

    public void onPlayerInteraction(PlayerEntity player) {
        if (this.world.isRemote) {
            return;
        }
        if (this.inGround && this.pickupAllowed && this.shake <= 0 && player.inventory.addStack(new ItemStack(Item.ARROW, 1))) {
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

