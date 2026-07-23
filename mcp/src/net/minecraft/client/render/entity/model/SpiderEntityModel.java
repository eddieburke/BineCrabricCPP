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
public class SpiderEntityModel
extends EntityModel {
    public ModelPart head;
    public ModelPart body0;
    public ModelPart body1;
    public ModelPart rightHindLeg;
    public ModelPart leftHindLeg;
    public ModelPart rightMiddleHindLeg;
    public ModelPart leftMiddleHindLeg;
    public ModelPart rightMiddleFrontLeg;
    public ModelPart leftMiddleFrontLeg;
    public ModelPart rightFrontLeg;
    public ModelPart leftFrontLeg;

    public SpiderEntityModel() {
        float f = 0.0f;
        int n = 15;
        this.head = new ModelPart(32, 4);
        this.head.addCuboid(-4.0f, -4.0f, -8.0f, 8, 8, 8, f);
        this.head.setPivot(0.0f, 0 + n, -3.0f);
        this.body0 = new ModelPart(0, 0);
        this.body0.addCuboid(-3.0f, -3.0f, -3.0f, 6, 6, 6, f);
        this.body0.setPivot(0.0f, n, 0.0f);
        this.body1 = new ModelPart(0, 12);
        this.body1.addCuboid(-5.0f, -4.0f, -6.0f, 10, 8, 12, f);
        this.body1.setPivot(0.0f, 0 + n, 9.0f);
        this.rightHindLeg = new ModelPart(18, 0);
        this.rightHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.rightHindLeg.setPivot(-4.0f, 0 + n, 2.0f);
        this.leftHindLeg = new ModelPart(18, 0);
        this.leftHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.leftHindLeg.setPivot(4.0f, 0 + n, 2.0f);
        this.rightMiddleHindLeg = new ModelPart(18, 0);
        this.rightMiddleHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.rightMiddleHindLeg.setPivot(-4.0f, 0 + n, 1.0f);
        this.leftMiddleHindLeg = new ModelPart(18, 0);
        this.leftMiddleHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.leftMiddleHindLeg.setPivot(4.0f, 0 + n, 1.0f);
        this.rightMiddleFrontLeg = new ModelPart(18, 0);
        this.rightMiddleFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.rightMiddleFrontLeg.setPivot(-4.0f, 0 + n, 0.0f);
        this.leftMiddleFrontLeg = new ModelPart(18, 0);
        this.leftMiddleFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.leftMiddleFrontLeg.setPivot(4.0f, 0 + n, 0.0f);
        this.rightFrontLeg = new ModelPart(18, 0);
        this.rightFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.rightFrontLeg.setPivot(-4.0f, 0 + n, -1.0f);
        this.leftFrontLeg = new ModelPart(18, 0);
        this.leftFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        this.leftFrontLeg.setPivot(4.0f, 0 + n, -1.0f);
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.head.render(scale);
        this.body0.render(scale);
        this.body1.render(scale);
        this.rightHindLeg.render(scale);
        this.leftHindLeg.render(scale);
        this.rightMiddleHindLeg.render(scale);
        this.leftMiddleHindLeg.render(scale);
        this.rightMiddleFrontLeg.render(scale);
        this.leftMiddleFrontLeg.render(scale);
        this.rightFrontLeg.render(scale);
        this.leftFrontLeg.render(scale);
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.head.yaw = headYaw / 57.295776f;
        this.head.pitch = headPitch / 57.295776f;
        float f = 0.7853982f;
        this.rightHindLeg.roll = -f;
        this.leftHindLeg.roll = f;
        this.rightMiddleHindLeg.roll = -f * 0.74f;
        this.leftMiddleHindLeg.roll = f * 0.74f;
        this.rightMiddleFrontLeg.roll = -f * 0.74f;
        this.leftMiddleFrontLeg.roll = f * 0.74f;
        this.rightFrontLeg.roll = -f;
        this.leftFrontLeg.roll = f;
        float f2 = -0.0f;
        float f3 = 0.3926991f;
        this.rightHindLeg.yaw = f3 * 2.0f + f2;
        this.leftHindLeg.yaw = -f3 * 2.0f - f2;
        this.rightMiddleHindLeg.yaw = f3 * 1.0f + f2;
        this.leftMiddleHindLeg.yaw = -f3 * 1.0f - f2;
        this.rightMiddleFrontLeg.yaw = -f3 * 1.0f + f2;
        this.leftMiddleFrontLeg.yaw = f3 * 1.0f - f2;
        this.rightFrontLeg.yaw = -f3 * 2.0f + f2;
        this.leftFrontLeg.yaw = f3 * 2.0f - f2;
        float f4 = -(MathHelper.cos(limbAngle * 0.6662f * 2.0f + 0.0f) * 0.4f) * limbDistance;
        float f5 = -(MathHelper.cos(limbAngle * 0.6662f * 2.0f + (float)Math.PI) * 0.4f) * limbDistance;
        float f6 = -(MathHelper.cos(limbAngle * 0.6662f * 2.0f + 1.5707964f) * 0.4f) * limbDistance;
        float f7 = -(MathHelper.cos(limbAngle * 0.6662f * 2.0f + 4.712389f) * 0.4f) * limbDistance;
        float f8 = Math.abs(MathHelper.sin(limbAngle * 0.6662f + 0.0f) * 0.4f) * limbDistance;
        float f9 = Math.abs(MathHelper.sin(limbAngle * 0.6662f + (float)Math.PI) * 0.4f) * limbDistance;
        float f10 = Math.abs(MathHelper.sin(limbAngle * 0.6662f + 1.5707964f) * 0.4f) * limbDistance;
        float f11 = Math.abs(MathHelper.sin(limbAngle * 0.6662f + 4.712389f) * 0.4f) * limbDistance;
        this.rightHindLeg.yaw += f4;
        this.leftHindLeg.yaw += -f4;
        this.rightMiddleHindLeg.yaw += f5;
        this.leftMiddleHindLeg.yaw += -f5;
        this.rightMiddleFrontLeg.yaw += f6;
        this.leftMiddleFrontLeg.yaw += -f6;
        this.rightFrontLeg.yaw += f7;
        this.leftFrontLeg.yaw += -f7;
        this.rightHindLeg.roll += f8;
        this.leftHindLeg.roll += -f8;
        this.rightMiddleHindLeg.roll += f9;
        this.leftMiddleHindLeg.roll += -f9;
        this.rightMiddleFrontLeg.roll += f10;
        this.leftMiddleFrontLeg.roll += -f10;
        this.rightFrontLeg.roll += f11;
        this.leftFrontLeg.roll += -f11;
    }
}

