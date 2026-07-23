/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity.model;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class ZombieEntityModel
extends BipedEntityModel {
    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        super.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        float f = MathHelper.sin(this.handSwingProgress * (float)Math.PI);
        float f2 = MathHelper.sin((1.0f - (1.0f - this.handSwingProgress) * (1.0f - this.handSwingProgress)) * (float)Math.PI);
        this.rightArm.roll = 0.0f;
        this.leftArm.roll = 0.0f;
        this.rightArm.yaw = -(0.1f - f * 0.6f);
        this.leftArm.yaw = 0.1f - f * 0.6f;
        this.rightArm.pitch = -1.5707964f;
        this.leftArm.pitch = -1.5707964f;
        this.rightArm.pitch -= f * 1.2f - f2 * 0.4f;
        this.leftArm.pitch -= f * 1.2f - f2 * 0.4f;
        this.rightArm.roll += MathHelper.cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        this.leftArm.roll -= MathHelper.cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        this.rightArm.pitch += MathHelper.sin(animationProgress * 0.067f) * 0.05f;
        this.leftArm.pitch -= MathHelper.sin(animationProgress * 0.067f) * 0.05f;
    }
}

