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
import net.minecraft.client.particle.Particle;
import net.minecraft.client.render.Tessellator;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class FlameParticle
extends Particle {
    private float initialScale;

    public FlameParticle(World arg, double d, double e, double f, double g, double h, double i) {
        super(arg, d, e, f, g, h, i);
        this.velocityX = this.velocityX * (double)0.01f + g;
        this.velocityY = this.velocityY * (double)0.01f + h;
        this.velocityZ = this.velocityZ * (double)0.01f + i;
        d += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.05f);
        e += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.05f);
        f += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.05f);
        this.initialScale = this.scale;
        this.blue = 1.0f;
        this.green = 1.0f;
        this.red = 1.0f;
        this.maxParticleAge = (int)(8.0 / (Math.random() * 0.8 + 0.2)) + 4;
        this.noClip = true;
        this.textureId = 48;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.particleAge + partialTicks) / (float)this.maxParticleAge;
        this.scale = this.initialScale * (1.0f - f * f * 0.5f);
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public float getBrightnessAtEyes(float tickDelta) {
        float f = ((float)this.particleAge + tickDelta) / (float)this.maxParticleAge;
        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        float f2 = super.getBrightnessAtEyes(tickDelta);
        return f2 * f + (1.0f - f);
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.96f;
        this.velocityY *= (double)0.96f;
        this.velocityZ *= (double)0.96f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }
}

