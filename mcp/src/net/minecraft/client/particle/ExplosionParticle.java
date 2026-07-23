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
public class ExplosionParticle
extends Particle {
    public ExplosionParticle(World arg, double d, double e, double f, double g, double h, double i) {
        super(arg, d, e, f, g, h, i);
        this.velocityX = g + (double)((float)(Math.random() * 2.0 - 1.0) * 0.05f);
        this.velocityY = h + (double)((float)(Math.random() * 2.0 - 1.0) * 0.05f);
        this.velocityZ = i + (double)((float)(Math.random() * 2.0 - 1.0) * 0.05f);
        this.green = this.blue = this.random.nextFloat() * 0.3f + 0.7f;
        this.red = this.blue;
        this.scale = this.random.nextFloat() * this.random.nextFloat() * 6.0f + 1.0f;
        this.maxParticleAge = (int)(16.0 / ((double)this.random.nextFloat() * 0.8 + 0.2)) + 2;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
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
        this.velocityX *= (double)0.9f;
        this.velocityY *= (double)0.9f;
        this.velocityZ *= (double)0.9f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }
}

