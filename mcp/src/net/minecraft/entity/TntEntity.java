/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class TntEntity
extends Entity {
    public int fuse = 0;

    public TntEntity(World arg) {
        super(arg);
        this.blocksSameBlockSpawning = true;
        this.setBoundingBoxSpacing(0.98f, 0.98f);
        this.standingEyeHeight = this.height / 2.0f;
    }

    public TntEntity(World world, double x, double y, double z) {
        this(world);
        this.setPosition(x, y, z);
        float f = (float)(Math.random() * 3.1415927410125732 * 2.0);
        this.velocityX = -MathHelper.sin(f * (float)Math.PI / 180.0f) * 0.02f;
        this.velocityY = 0.2f;
        this.velocityZ = -MathHelper.cos(f * (float)Math.PI / 180.0f) * 0.02f;
        this.fuse = 80;
        this.prevX = x;
        this.prevY = y;
        this.prevZ = z;
    }

    protected void initDataTracker() {
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    public boolean isCollidable() {
        return !this.dead;
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.velocityY -= (double)0.04f;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.98f;
        this.velocityY *= (double)0.98f;
        this.velocityZ *= (double)0.98f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
            this.velocityY *= -0.5;
        }
        if (this.fuse-- <= 0) {
            if (!this.world.isRemote) {
                this.markDead();
                this.explode();
            } else {
                this.markDead();
            }
        } else {
            this.world.addParticle("smoke", this.x, this.y + 0.5, this.z, 0.0, 0.0, 0.0);
        }
    }

    private void explode() {
        float f = 4.0f;
        this.world.createExplosion(null, this.x, this.y, this.z, f);
    }

    protected void writeNbt(NbtCompound nbt) {
        nbt.putByte("Fuse", (byte)this.fuse);
    }

    protected void readNbt(NbtCompound nbt) {
        this.fuse = nbt.getByte("Fuse");
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }
}

