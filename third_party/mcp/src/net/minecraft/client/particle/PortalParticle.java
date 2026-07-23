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
public class PortalParticle
extends Particle {
    private float initialScale;
    private double initialX;
    private double initialY;
    private double initialZ;

    public PortalParticle(World arg, double d, double e, double f, double g, double h, double i) {
        super(arg, d, e, f, g, h, i);
        this.velocityX = g;
        this.velocityY = h;
        this.velocityZ = i;
        this.initialX = this.x = d;
        this.initialY = this.y = e;
        this.initialZ = this.z = f;
        float f2 = this.random.nextFloat() * 0.6f + 0.4f;
        this.initialScale = this.scale = this.random.nextFloat() * 0.2f + 0.5f;
        this.green = this.blue = 1.0f * f2;
        this.red = this.blue;
        this.green *= 0.3f;
        this.red *= 0.9f;
        this.maxParticleAge = (int)(Math.random() * 10.0) + 40;
        this.noClip = true;
        this.textureId = (int)(Math.random() * 8.0);
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.particleAge + partialTicks) / (float)this.maxParticleAge;
        f = 1.0f - f;
        f *= f;
        f = 1.0f - f;
        this.scale = this.initialScale * f;
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public float getBrightnessAtEyes(float tickDelta) {
        float f = super.getBrightnessAtEyes(tickDelta);
        float f2 = (float)this.particleAge / (float)this.maxParticleAge;
        f2 *= f2;
        f2 *= f2;
        return f * (1.0f - f2) + f2;
    }

    public void tick() {
        float f;
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        float f2 = f = (float)this.particleAge / (float)this.maxParticleAge;
        f = -f + f * f * 2.0f;
        f = 1.0f - f;
        this.x = this.initialX + this.velocityX * (double)f;
        this.y = this.initialY + this.velocityY * (double)f + (double)(1.0f - f2);
        this.z = this.initialZ + this.velocityZ * (double)f;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
    }
}

