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
import net.minecraft.entity.passive.SquidEntity;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class SquidEntityRenderer
extends LivingEntityRenderer {
    public SquidEntityRenderer(EntityModel arg, float f) {
        super(arg, f);
    }

    public void render(SquidEntity arg, double d, double e, double f, float g, float h) {
        super.render(arg, d, e, f, g, h);
    }

    protected void applyHandSwingRotation(SquidEntity arg, float f, float g, float h) {
        float f2 = arg.lastTiltAngle + (arg.tiltAngle - arg.lastTiltAngle) * h;
        float f3 = arg.lastRollAngle + (arg.rollAngle - arg.lastRollAngle) * h;
        GL11.glTranslatef(0.0f, 0.5f, 0.0f);
        GL11.glRotatef(180.0f - g, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(f2, 1.0f, 0.0f, 0.0f);
        GL11.glRotatef(f3, 0.0f, 1.0f, 0.0f);
        GL11.glTranslatef(0.0f, -1.2f, 0.0f);
    }

    protected void applyScale(SquidEntity arg, float f) {
    }

    protected float getHeadBob(SquidEntity arg, float f) {
        float f2 = arg.lastTentacleAngle + (arg.tentacleAngle - arg.lastTentacleAngle) * f;
        return f2;
    }
}

