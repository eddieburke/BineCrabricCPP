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
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.material.Material;
import net.minecraft.client.particle.Particle;
import net.minecraft.client.render.Tessellator;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class RainSplashParticle
extends Particle {
    public RainSplashParticle(World world, double x, double y, double z) {
        super(world, x, y, z, 0.0, 0.0, 0.0);
        this.velocityX *= (double)0.3f;
        this.velocityY = (float)Math.random() * 0.2f + 0.1f;
        this.velocityZ *= (double)0.3f;
        this.red = 1.0f;
        this.green = 1.0f;
        this.blue = 1.0f;
        this.textureId = 19 + this.random.nextInt(4);
        this.setBoundingBoxSpacing(0.01f, 0.01f);
        this.gravityStrength = 0.06f;
        this.maxParticleAge = (int)(8.0 / (Math.random() * 0.8 + 0.2));
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        super.render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    public void tick() {
        double d;
        Material material;
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.velocityY -= (double)this.gravityStrength;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.98f;
        this.velocityY *= (double)0.98f;
        this.velocityZ *= (double)0.98f;
        if (this.maxParticleAge-- <= 0) {
            this.markDead();
        }
        if (this.onGround) {
            if (Math.random() < 0.5) {
                this.markDead();
            }
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
        }
        if (((material = this.world.getMaterial(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z))).isFluid() || material.isSolid()) && this.y < (d = (double)((float)(MathHelper.floor(this.y) + 1) - LiquidBlock.getFluidHeightFromMeta(this.world.getBlockMeta(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)))))) {
            this.markDead();
        }
    }
}

