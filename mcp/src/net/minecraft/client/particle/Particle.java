/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.particle;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.Tessellator;
import net.minecraft.entity.Entity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class Particle
extends Entity {
    protected int textureId;
    protected float prevU;
    protected float prevV;
    protected int particleAge = 0;
    protected int maxParticleAge = 0;
    protected float scale;
    protected float gravityStrength;
    protected float red;
    protected float green;
    protected float blue;
    public static double xOffset;
    public static double yOffset;
    public static double zOffset;

    public Particle(World world, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
        super(world);
        this.setBoundingBoxSpacing(0.2f, 0.2f);
        this.standingEyeHeight = this.height / 2.0f;
        this.setPosition(x, y, z);
        this.blue = 1.0f;
        this.green = 1.0f;
        this.red = 1.0f;
        this.velocityX = velocityX + (double)((float)(Math.random() * 2.0 - 1.0) * 0.4f);
        this.velocityY = velocityY + (double)((float)(Math.random() * 2.0 - 1.0) * 0.4f);
        this.velocityZ = velocityZ + (double)((float)(Math.random() * 2.0 - 1.0) * 0.4f);
        float f = (float)(Math.random() + Math.random() + 1.0) * 0.15f;
        float f2 = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityY * this.velocityY + this.velocityZ * this.velocityZ);
        this.velocityX = this.velocityX / (double)f2 * (double)f * (double)0.4f;
        this.velocityY = this.velocityY / (double)f2 * (double)f * (double)0.4f + (double)0.1f;
        this.velocityZ = this.velocityZ / (double)f2 * (double)f * (double)0.4f;
        this.prevU = this.random.nextFloat() * 3.0f;
        this.prevV = this.random.nextFloat() * 3.0f;
        this.scale = (this.random.nextFloat() * 0.5f + 0.5f) * 2.0f;
        this.maxParticleAge = (int)(4.0f / (this.random.nextFloat() * 0.9f + 0.1f));
        this.particleAge = 0;
    }

    public Particle multiplyVelocity(float factor) {
        this.velocityX *= (double)factor;
        this.velocityY = (this.velocityY - (double)0.1f) * (double)factor + (double)0.1f;
        this.velocityZ *= (double)factor;
        return this;
    }

    public Particle setScale(float scale) {
        this.setBoundingBoxSpacing(0.2f * scale, 0.2f * scale);
        this.scale *= scale;
        return this;
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    protected void initDataTracker() {
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
        this.velocityY -= 0.04 * (double)this.gravityStrength;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.98f;
        this.velocityY *= (double)0.98f;
        this.velocityZ *= (double)0.98f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = (float)(this.textureId % 16) / 16.0f;
        float f2 = f + 0.0624375f;
        float f3 = (float)(this.textureId / 16) / 16.0f;
        float f4 = f3 + 0.0624375f;
        float f5 = 0.1f * this.scale;
        float f6 = (float)(this.prevX + (this.x - this.prevX) * (double)partialTicks - xOffset);
        float f7 = (float)(this.prevY + (this.y - this.prevY) * (double)partialTicks - yOffset);
        float f8 = (float)(this.prevZ + (this.z - this.prevZ) * (double)partialTicks - zOffset);
        float f9 = this.getBrightnessAtEyes(partialTicks);
        tessellator.color(this.red * f9, this.green * f9, this.blue * f9);
        tessellator.vertex(f6 - horizontalSize * f5 - widthOffset * f5, f7 - verticalSize * f5, f8 - depthSize * f5 - heightOffset * f5, f2, f4);
        tessellator.vertex(f6 - horizontalSize * f5 + widthOffset * f5, f7 + verticalSize * f5, f8 - depthSize * f5 + heightOffset * f5, f2, f3);
        tessellator.vertex(f6 + horizontalSize * f5 + widthOffset * f5, f7 + verticalSize * f5, f8 + depthSize * f5 + heightOffset * f5, f, f3);
        tessellator.vertex(f6 + horizontalSize * f5 - widthOffset * f5, f7 - verticalSize * f5, f8 + depthSize * f5 - heightOffset * f5, f, f4);
    }

    public int getGroup() {
        return 0;
    }

    public void writeNbt(NbtCompound nbt) {
    }

    public void readNbt(NbtCompound nbt) {
    }
}

