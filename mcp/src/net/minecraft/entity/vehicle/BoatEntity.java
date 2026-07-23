/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.vehicle;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class BoatEntity
extends Entity {
    public int damageWobbleStrength = 0;
    public int damageWobbleTicks = 0;
    public int damageWobbleSide = 1;
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

    public BoatEntity(World arg) {
        super(arg);
        this.blocksSameBlockSpawning = true;
        this.setBoundingBoxSpacing(1.5f, 0.6f);
        this.standingEyeHeight = this.height / 2.0f;
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    protected void initDataTracker() {
    }

    public Box getCollisionAgainstShape(Entity other) {
        return other.boundingBox;
    }

    public Box getBoundingBox() {
        return this.boundingBox;
    }

    public boolean isPushable() {
        return true;
    }

    public BoatEntity(World world, double x, double y, double z) {
        this(world);
        this.setPosition(x, y + (double)this.standingEyeHeight, z);
        this.velocityX = 0.0;
        this.velocityY = 0.0;
        this.velocityZ = 0.0;
        this.prevX = x;
        this.prevY = y;
        this.prevZ = z;
    }

    public double getPassengerRidingHeight() {
        return (double)this.height * 0.0 - (double)0.3f;
    }

    public boolean damage(Entity damageSource, int amount) {
        if (this.world.isRemote || this.dead) {
            return true;
        }
        this.damageWobbleSide = -this.damageWobbleSide;
        this.damageWobbleTicks = 10;
        this.damageWobbleStrength += amount * 10;
        this.scheduleVelocityUpdate();
        if (this.damageWobbleStrength > 40) {
            int n;
            if (this.passenger != null) {
                this.passenger.setVehicle(this);
            }
            for (n = 0; n < 3; ++n) {
                this.dropItem(Block.PLANKS.id, 1, 0.0f);
            }
            for (n = 0; n < 2; ++n) {
                this.dropItem(Item.STICK.id, 1, 0.0f);
            }
            this.markDead();
        }
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public void animateHurt() {
        this.damageWobbleSide = -this.damageWobbleSide;
        this.damageWobbleTicks = 10;
        this.damageWobbleStrength += this.damageWobbleStrength * 10;
    }

    public boolean isCollidable() {
        return !this.dead;
    }

    @Environment(value=EnvType.CLIENT)
    public void setPositionAndAnglesAvoidEntities(double x, double y, double z, float pitch, float yaw, int interpolationSteps) {
        this.clientX = x;
        this.clientY = y;
        this.clientZ = z;
        this.clientPitch = pitch;
        this.clientYaw = yaw;
        this.clientInterpolationSteps = interpolationSteps + 4;
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

    public void tick() {
        double d;
        double d2;
        double d3;
        super.tick();
        if (this.damageWobbleTicks > 0) {
            --this.damageWobbleTicks;
        }
        if (this.damageWobbleStrength > 0) {
            --this.damageWobbleStrength;
        }
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        int n = 5;
        double d4 = 0.0;
        for (int i = 0; i < n; ++i) {
            double d5 = this.boundingBox.minY + (this.boundingBox.maxY - this.boundingBox.minY) * (double)(i + 0) / (double)n - 0.125;
            double d6 = this.boundingBox.minY + (this.boundingBox.maxY - this.boundingBox.minY) * (double)(i + 1) / (double)n - 0.125;
            Box box = Box.createCached(this.boundingBox.minX, d5, this.boundingBox.minZ, this.boundingBox.maxX, d6, this.boundingBox.maxZ);
            if (!this.world.isFluidInBox(box, Material.WATER)) continue;
            d4 += 1.0 / (double)n;
        }
        if (this.world.isRemote) {
            if (this.clientInterpolationSteps > 0) {
                double d7;
                double d8 = this.x + (this.clientX - this.x) / (double)this.clientInterpolationSteps;
                double d9 = this.y + (this.clientY - this.y) / (double)this.clientInterpolationSteps;
                double d10 = this.z + (this.clientZ - this.z) / (double)this.clientInterpolationSteps;
                for (d7 = this.clientPitch - (double)this.yaw; d7 < -180.0; d7 += 360.0) {
                }
                while (d7 >= 180.0) {
                    d7 -= 360.0;
                }
                this.yaw = (float)((double)this.yaw + d7 / (double)this.clientInterpolationSteps);
                this.pitch = (float)((double)this.pitch + (this.clientYaw - (double)this.pitch) / (double)this.clientInterpolationSteps);
                --this.clientInterpolationSteps;
                this.setPosition(d8, d9, d10);
                this.setRotation(this.yaw, this.pitch);
            } else {
                double d11 = this.x + this.velocityX;
                double d12 = this.y + this.velocityY;
                double d13 = this.z + this.velocityZ;
                this.setPosition(d11, d12, d13);
                if (this.onGround) {
                    this.velocityX *= 0.5;
                    this.velocityY *= 0.5;
                    this.velocityZ *= 0.5;
                }
                this.velocityX *= (double)0.99f;
                this.velocityY *= (double)0.95f;
                this.velocityZ *= (double)0.99f;
            }
            return;
        }
        if (d4 < 1.0) {
            double d14 = d4 * 2.0 - 1.0;
            this.velocityY += (double)0.04f * d14;
        } else {
            if (this.velocityY < 0.0) {
                this.velocityY /= 2.0;
            }
            this.velocityY += (double)0.007f;
        }
        if (this.passenger != null) {
            this.velocityX += this.passenger.velocityX * 0.2;
            this.velocityZ += this.passenger.velocityZ * 0.2;
        }
        if (this.velocityX < -(d3 = 0.4)) {
            this.velocityX = -d3;
        }
        if (this.velocityX > d3) {
            this.velocityX = d3;
        }
        if (this.velocityZ < -d3) {
            this.velocityZ = -d3;
        }
        if (this.velocityZ > d3) {
            this.velocityZ = d3;
        }
        if (this.onGround) {
            this.velocityX *= 0.5;
            this.velocityY *= 0.5;
            this.velocityZ *= 0.5;
        }
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        double d15 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
        if (d15 > 0.15) {
            double d16 = Math.cos((double)this.yaw * Math.PI / 180.0);
            d2 = Math.sin((double)this.yaw * Math.PI / 180.0);
            int n2 = 0;
            while ((double)n2 < 1.0 + d15 * 60.0) {
                double d17;
                double d18;
                double d19 = this.random.nextFloat() * 2.0f - 1.0f;
                double d20 = (double)(this.random.nextInt(2) * 2 - 1) * 0.7;
                if (this.random.nextBoolean()) {
                    d18 = this.x - d16 * d19 * 0.8 + d2 * d20;
                    d17 = this.z - d2 * d19 * 0.8 - d16 * d20;
                    this.world.addParticle("splash", d18, this.y - 0.125, d17, this.velocityX, this.velocityY, this.velocityZ);
                } else {
                    d18 = this.x + d16 + d2 * d19 * 0.7;
                    d17 = this.z + d2 - d16 * d19 * 0.7;
                    this.world.addParticle("splash", d18, this.y - 0.125, d17, this.velocityX, this.velocityY, this.velocityZ);
                }
                ++n2;
            }
        }
        if (this.horizontalCollision && d15 > 0.15) {
            if (!this.world.isRemote) {
                int n3;
                this.markDead();
                for (n3 = 0; n3 < 3; ++n3) {
                    this.dropItem(Block.PLANKS.id, 1, 0.0f);
                }
                for (n3 = 0; n3 < 2; ++n3) {
                    this.dropItem(Item.STICK.id, 1, 0.0f);
                }
            }
        } else {
            this.velocityX *= (double)0.99f;
            this.velocityY *= (double)0.95f;
            this.velocityZ *= (double)0.99f;
        }
        this.pitch = 0.0f;
        double d21 = this.yaw;
        d2 = this.prevX - this.x;
        double d22 = this.prevZ - this.z;
        if (d2 * d2 + d22 * d22 > 0.001) {
            d21 = (float)(Math.atan2(d22, d2) * 180.0 / Math.PI);
        }
        for (d = d21 - (double)this.yaw; d >= 180.0; d -= 360.0) {
        }
        while (d < -180.0) {
            d += 360.0;
        }
        if (d > 20.0) {
            d = 20.0;
        }
        if (d < -20.0) {
            d = -20.0;
        }
        this.yaw = (float)((double)this.yaw + d);
        this.setRotation(this.yaw, this.pitch);
        List list = this.world.getEntities(this, this.boundingBox.expand(0.2f, 0.0, 0.2f));
        if (list != null && list.size() > 0) {
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                if (entity == this.passenger || !entity.isPushable() || !(entity instanceof BoatEntity)) continue;
                entity.onCollision(this);
            }
        }
        for (int i = 0; i < 4; ++i) {
            int n4;
            int n5;
            int n6 = MathHelper.floor(this.x + ((double)(i % 2) - 0.5) * 0.8);
            if (this.world.getBlockId(n6, n5 = MathHelper.floor(this.y), n4 = MathHelper.floor(this.z + ((double)(i / 2) - 0.5) * 0.8)) != Block.SNOW.id) continue;
            this.world.setBlock(n6, n5, n4, 0);
        }
        if (this.passenger != null && this.passenger.dead) {
            this.passenger = null;
        }
    }

    public void updatePassengerPosition() {
        if (this.passenger == null) {
            return;
        }
        double d = Math.cos((double)this.yaw * Math.PI / 180.0) * 0.4;
        double d2 = Math.sin((double)this.yaw * Math.PI / 180.0) * 0.4;
        this.passenger.setPosition(this.x + d, this.y + this.getPassengerRidingHeight() + this.passenger.getStandingEyeHeight(), this.z + d2);
    }

    protected void writeNbt(NbtCompound nbt) {
    }

    protected void readNbt(NbtCompound nbt) {
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }

    public boolean interact(PlayerEntity player) {
        if (this.passenger != null && this.passenger instanceof PlayerEntity && this.passenger != player) {
            return true;
        }
        if (!this.world.isRemote) {
            player.setVehicle(this);
        }
        return true;
    }
}

