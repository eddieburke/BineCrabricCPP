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
public class CreeperEntityModel
extends EntityModel {
    public ModelPart head;
    public ModelPart hat;
    public ModelPart body;
    public ModelPart rightHindLeg;
    public ModelPart leftHindLeg;
    public ModelPart rightFrontLeg;
    public ModelPart leftFrontLeg;

    public CreeperEntityModel() {
        this(0.0f);
    }

    public CreeperEntityModel(float dilation) {
        int n = 4;
        this.head = new ModelPart(0, 0);
        this.head.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation);
        this.head.setPivot(0.0f, n, 0.0f);
        this.hat = new ModelPart(32, 0);
        this.hat.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation + 0.5f);
        this.hat.setPivot(0.0f, n, 0.0f);
        this.body = new ModelPart(16, 16);
        this.body.addCuboid(-4.0f, 0.0f, -2.0f, 8, 12, 4, dilation);
        this.body.setPivot(0.0f, n, 0.0f);
        this.rightHindLeg = new ModelPart(0, 16);
        this.rightHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
        this.rightHindLeg.setPivot(-2.0f, 12 + n, 4.0f);
        this.leftHindLeg = new ModelPart(0, 16);
        this.leftHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
        this.leftHindLeg.setPivot(2.0f, 12 + n, 4.0f);
        this.rightFrontLeg = new ModelPart(0, 16);
        this.rightFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
        this.rightFrontLeg.setPivot(-2.0f, 12 + n, -4.0f);
        this.leftFrontLeg = new ModelPart(0, 16);
        this.leftFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
        this.leftFrontLeg.setPivot(2.0f, 12 + n, -4.0f);
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
        this.head.yaw = headYaw / 57.295776f;
        this.head.pitch = headPitch / 57.295776f;
        this.rightHindLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
        this.leftHindLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.rightFrontLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.leftFrontLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    }
}

