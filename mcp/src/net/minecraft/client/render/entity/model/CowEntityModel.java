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
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.entity.model.QuadrupedEntityModel;

@Environment(value=EnvType.CLIENT)
public class CowEntityModel
extends QuadrupedEntityModel {
    ModelPart udder;
    ModelPart rightHorn;
    ModelPart leftHorn;

    public CowEntityModel() {
        super(12, 0.0f);
        this.head = new ModelPart(0, 0);
        this.head.addCuboid(-4.0f, -4.0f, -6.0f, 8, 8, 6, 0.0f);
        this.head.setPivot(0.0f, 4.0f, -8.0f);
        this.rightHorn = new ModelPart(22, 0);
        this.rightHorn.addCuboid(-4.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
        this.rightHorn.setPivot(0.0f, 3.0f, -7.0f);
        this.leftHorn = new ModelPart(22, 0);
        this.leftHorn.addCuboid(3.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
        this.leftHorn.setPivot(0.0f, 3.0f, -7.0f);
        this.udder = new ModelPart(52, 0);
        this.udder.addCuboid(-2.0f, -3.0f, 0.0f, 4, 6, 2, 0.0f);
        this.udder.setPivot(0.0f, 14.0f, 6.0f);
        this.udder.pitch = 1.5707964f;
        this.body = new ModelPart(18, 4);
        this.body.addCuboid(-6.0f, -10.0f, -7.0f, 12, 18, 10, 0.0f);
        this.body.setPivot(0.0f, 5.0f, 2.0f);
        this.rightHindLeg.pivotX -= 1.0f;
        this.leftHindLeg.pivotX += 1.0f;
        this.rightHindLeg.pivotZ += 0.0f;
        this.leftHindLeg.pivotZ += 0.0f;
        this.rightFrontLeg.pivotX -= 1.0f;
        this.leftFrontLeg.pivotX += 1.0f;
        this.rightFrontLeg.pivotZ -= 1.0f;
        this.leftFrontLeg.pivotZ -= 1.0f;
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        super.render(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.rightHorn.render(scale);
        this.leftHorn.render(scale);
        this.udder.render(scale);
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        super.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.rightHorn.yaw = this.head.yaw;
        this.rightHorn.pitch = this.head.pitch;
        this.leftHorn.yaw = this.head.yaw;
        this.leftHorn.pitch = this.head.pitch;
    }
}

