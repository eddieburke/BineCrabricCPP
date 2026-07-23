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
public class FireSmokeParticle
extends Particle {
    float initialScale;

    public FireSmokeParticle(World arg, double d, double e, double f, double g, double h, double i) {
        this(arg, d, e, f, g, h, i, 1.0f);
    }

    public FireSmokeParticle(World world, double x, double y, double z, double velocityX, double velocityY, double velocityZ, float scale) {
        super(world, x, y, z, 0.0, 0.0, 0.0);
        this.velocityX *= (double)0.1f;
        this.velocityY *= (double)0.1f;
        this.velocityZ *= (double)0.1f;
        this.velocityX += velocityX;
        this.velocityY += velocityY;
        this.velocityZ += velocityZ;
        this.green = this.blue = (float)(Math.random() * (double)0.3f);
        this.red = this.blue;
        this.scale *= 0.75f;
        this.scale *= scale;
        this.initialScale = this.scale;
        this.maxParticleAge = (int)(8.0 / (Math.random() * 0.8 + 0.2));
        this.maxParticleAge = (int)((float)this.maxParticleAge * scale);
        this.noClip = false;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.particleAge + partialTicks) / (float)this.maxParticleAge * 32.0f;
        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        this.scale = this.initialScale * f;
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
        this.textureId = 7 - this.particleAge * 8 / this.maxParticleAge;
        this.velocityY += 0.004;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        if (this.y == this.prevY) {
            this.velocityX *= 1.1;
            this.velocityZ *= 1.1;
        }
        this.velocityX *= (double)0.96f;
        this.velocityY *= (double)0.96f;
        this.velocityZ *= (double)0.96f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }
}

