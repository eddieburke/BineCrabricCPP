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
public class LavaEmberParticle
extends Particle {
    private float initialScale;

    public LavaEmberParticle(World arg, double d, double e, double f) {
        super(arg, d, e, f, 0.0, 0.0, 0.0);
        this.velocityX *= (double)0.8f;
        this.velocityY *= (double)0.8f;
        this.velocityZ *= (double)0.8f;
        this.velocityY = this.random.nextFloat() * 0.4f + 0.05f;
        this.blue = 1.0f;
        this.green = 1.0f;
        this.red = 1.0f;
        this.scale *= this.random.nextFloat() * 2.0f + 0.2f;
        this.initialScale = this.scale;
        this.maxParticleAge = (int)(16.0 / (Math.random() * 0.8 + 0.2));
        this.noClip = false;
        this.textureId = 49;
    }

    public float getBrightnessAtEyes(float tickDelta) {
        return 1.0f;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.particleAge + partialTicks) / (float)this.maxParticleAge;
        this.scale = this.initialScale * (1.0f - f * f);
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
        float f = (float)this.particleAge / (float)this.maxParticleAge;
        if (this.random.nextFloat() > f) {
            this.world.addParticle("smoke", this.x, this.y, this.z, this.velocityX, this.velocityY, this.velocityZ);
        }
        this.velocityY -= 0.03;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.999f;
        this.velocityY *= (double)0.999f;
        this.velocityZ *= (double)0.999f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }
}

