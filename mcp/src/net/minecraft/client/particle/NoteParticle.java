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
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class NoteParticle
extends Particle {
    float startScale;

    public NoteParticle(World arg, double d, double e, double f, double g, double h, double i) {
        this(arg, d, e, f, g, h, i, 2.0f);
    }

    public NoteParticle(World world, double x, double y, double z, double noteColorModifier, double h, double i, float scale) {
        super(world, x, y, z, 0.0, 0.0, 0.0);
        this.velocityX *= (double)0.01f;
        this.velocityY *= (double)0.01f;
        this.velocityZ *= (double)0.01f;
        this.velocityY += 0.2;
        this.red = MathHelper.sin(((float)noteColorModifier + 0.0f) * (float)Math.PI * 2.0f) * 0.65f + 0.35f;
        this.green = MathHelper.sin(((float)noteColorModifier + 0.33333334f) * (float)Math.PI * 2.0f) * 0.65f + 0.35f;
        this.blue = MathHelper.sin(((float)noteColorModifier + 0.6666667f) * (float)Math.PI * 2.0f) * 0.65f + 0.35f;
        this.scale *= 0.75f;
        this.scale *= scale;
        this.startScale = this.scale;
        this.maxParticleAge = 6;
        this.noClip = false;
        this.textureId = 64;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.particleAge + partialTicks) / (float)this.maxParticleAge * 32.0f;
        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        this.scale = this.startScale * f;
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        if (this.particleAge++ >= this.maxParticleAge) {
            this.markDead();
        }
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        if (this.y == this.prevY) {
            this.velocityX *= 1.1;
            this.velocityZ *= 1.1;
        }
        this.velocityX *= (double)0.66f;
        this.velocityY *= (double)0.66f;
        this.velocityZ *= (double)0.66f;
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
    }
}

