/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.entity.passive.ChickenEntity;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class ChickenEntityRenderer
extends LivingEntityRenderer {
    public ChickenEntityRenderer(EntityModel arg, float f) {
        super(arg, f);
    }

    public void render(ChickenEntity arg, double d, double e, double f, float g, float h) {
        super.render(arg, d, e, f, g, h);
    }

    protected float getHeadBob(ChickenEntity arg, float f) {
        float f2 = arg.prevFlapProgress + (arg.flapProgress - arg.prevFlapProgress) * f;
        float f3 = arg.prevMaxWingDeviation + (arg.maxWingDeviation - arg.prevMaxWingDeviation) * f;
        return (MathHelper.sin(f2) + 1.0f) * f3;
    }
}

