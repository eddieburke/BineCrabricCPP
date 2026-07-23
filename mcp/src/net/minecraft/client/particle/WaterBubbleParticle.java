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
import net.minecraft.block.material.Material;
import net.minecraft.client.particle.Particle;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class WaterBubbleParticle
extends Particle {
    public WaterBubbleParticle(World arg, double d, double e, double f, double g, double h, double i) {
        super(arg, d, e, f, g, h, i);
        this.red = 1.0f;
        this.green = 1.0f;
        this.blue = 1.0f;
        this.textureId = 32;
        this.setBoundingBoxSpacing(0.02f, 0.02f);
        this.scale *= this.random.nextFloat() * 0.6f + 0.2f;
        this.velocityX = g * (double)0.2f + (double)((float)(Math.random() * 2.0 - 1.0) * 0.02f);
        this.velocityY = h * (double)0.2f + (double)((float)(Math.random() * 2.0 - 1.0) * 0.02f);
        this.velocityZ = i * (double)0.2f + (double)((float)(Math.random() * 2.0 - 1.0) * 0.02f);
        this.maxParticleAge = (int)(8.0 / (Math.random() * 0.8 + 0.2));
    }

    public void tick() {
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.velocityY += 0.002;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.85f;
        this.velocityY *= (double)0.85f;
        this.velocityZ *= (double)0.85f;
        if (this.world.getMaterial(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)) != Material.WATER) {
            this.markDead();
        }
        if (this.maxParticleAge-- <= 0) {
            this.markDead();
        }
    }
}

