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
import net.minecraft.client.particle.RainSplashParticle;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class WaterSplashParticle
extends RainSplashParticle {
    public WaterSplashParticle(World arg, double d, double e, double f, double g, double h, double i) {
        super(arg, d, e, f);
        this.gravityStrength = 0.04f;
        ++this.textureId;
        if (h == 0.0 && (g != 0.0 || i != 0.0)) {
            this.velocityX = g;
            this.velocityY = h + 0.1;
            this.velocityZ = i;
        }
    }
}

