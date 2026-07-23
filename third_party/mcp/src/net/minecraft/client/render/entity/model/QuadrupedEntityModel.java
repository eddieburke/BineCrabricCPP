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
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class QuadrupedEntityModel
extends EntityModel {
    public ModelPart head = new ModelPart(0, 0);
    public ModelPart body;
    public ModelPart rightHindLeg;
    public ModelPart leftHindLeg;
    public ModelPart rightFrontLeg;
    public ModelPart leftFrontLeg;

    public QuadrupedEntityModel(int stanceWidth, float dilation) {
        this.head.addCuboid(-4.0f, -4.0f, -8.0f, 8, 8, 8, dilation);
        this.head.setPivot(0.0f, 18 - stanceWidth, -6.0f);
        this.body = new ModelPart(28, 8);
        this.body.addCuboid(-5.0f, -10.0f, -7.0f, 10, 16, 8, dilation);
        this.body.setPivot(0.0f, 17 - stanceWidth, 2.0f);
        this.rightHindLeg = new ModelPart(0, 16);
        this.rightHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
        this.rightHindLeg.setPivot(-3.0f, 24 - stanceWidth, 7.0f);
        this.leftHindLeg = new ModelPart(0, 16);
        this.leftHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
        this.leftHindLeg.setPivot(3.0f, 24 - stanceWidth, 7.0f);
        this.rightFrontLeg = new ModelPart(0, 16);
        this.rightFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
        this.rightFrontLeg.setPivot(-3.0f, 24 - stanceWidth, -5.0f);
        this.leftFrontLeg = new ModelPart(0, 16);
        this.leftFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
        this.leftFrontLeg.setPivot(3.0f, 24 - stanceWidth, -5.0f);
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.head.render(scale);
        this.body.render(scale);
        this.rightHindLeg.render(scale);
        this.leftHindLeg.render(scale);
        this.rightFrontLeg.render(scale);
        this.leftFrontLeg.render(scale);
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.head.pitch = headPitch / 57.295776f;
        this.head.yaw = headYaw / 57.295776f;
        this.body.pitch = 1.5707964f;
        this.rightHindLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
        this.leftHindLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.rightFrontLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.leftFrontLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    }
}

