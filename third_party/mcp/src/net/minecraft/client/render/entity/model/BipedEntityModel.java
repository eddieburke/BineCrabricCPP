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
public class BipedEntityModel
extends EntityModel {
    public ModelPart head;
    public ModelPart hat;
    public ModelPart body;
    public ModelPart rightArm;
    public ModelPart leftArm;
    public ModelPart rightLeg;
    public ModelPart leftLeg;
    public ModelPart ears;
    public ModelPart cape = new ModelPart(0, 0);
    public boolean leftArmPose = false;
    public boolean rightArmPose = false;
    public boolean sneaking = false;

    public BipedEntityModel() {
        this(0.0f);
    }

    public BipedEntityModel(float dilation) {
        this(dilation, 0.0f);
    }

    public BipedEntityModel(float dilation, float pivotOffsetY) {
        this.cape.addCuboid(-5.0f, 0.0f, -1.0f, 10, 16, 1, dilation);
        this.ears = new ModelPart(24, 0);
        this.ears.addCuboid(-3.0f, -6.0f, -1.0f, 6, 6, 1, dilation);
        this.head = new ModelPart(0, 0);
        this.head.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation);
        this.head.setPivot(0.0f, 0.0f + pivotOffsetY, 0.0f);
        this.hat = new ModelPart(32, 0);
        this.hat.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation + 0.5f);
        this.hat.setPivot(0.0f, 0.0f + pivotOffsetY, 0.0f);
        this.body = new ModelPart(16, 16);
        this.body.addCuboid(-4.0f, 0.0f, -2.0f, 8, 12, 4, dilation);
        this.body.setPivot(0.0f, 0.0f + pivotOffsetY, 0.0f);
        this.rightArm = new ModelPart(40, 16);
        this.rightArm.addCuboid(-3.0f, -2.0f, -2.0f, 4, 12, 4, dilation);
        this.rightArm.setPivot(-5.0f, 2.0f + pivotOffsetY, 0.0f);
        this.leftArm = new ModelPart(40, 16);
        this.leftArm.mirror = true;
        this.leftArm.addCuboid(-1.0f, -2.0f, -2.0f, 4, 12, 4, dilation);
        this.leftArm.setPivot(5.0f, 2.0f + pivotOffsetY, 0.0f);
        this.rightLeg = new ModelPart(0, 16);
        this.rightLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 12, 4, dilation);
        this.rightLeg.setPivot(-2.0f, 12.0f + pivotOffsetY, 0.0f);
        this.leftLeg = new ModelPart(0, 16);
        this.leftLeg.mirror = true;
        this.leftLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 12, 4, dilation);
        this.leftLeg.setPivot(2.0f, 12.0f + pivotOffsetY, 0.0f);
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.head.render(scale);
        this.body.render(scale);
        this.rightArm.render(scale);
        this.leftArm.render(scale);
        this.rightLeg.render(scale);
        this.leftLeg.render(scale);
        this.hat.render(scale);
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.head.yaw = headYaw / 57.295776f;
        this.head.pitch = headPitch / 57.295776f;
        this.hat.yaw = this.head.yaw;
        this.hat.pitch = this.head.pitch;
        this.rightArm.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 2.0f * limbDistance * 0.5f;
        this.leftArm.pitch = MathHelper.cos(limbAngle * 0.6662f) * 2.0f * limbDistance * 0.5f;
        this.rightArm.roll = 0.0f;
        this.leftArm.roll = 0.0f;
        this.rightLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
        this.leftLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.rightLeg.yaw = 0.0f;
        this.leftLeg.yaw = 0.0f;
        if (this.riding) {
            this.rightArm.pitch += -0.62831855f;
            this.leftArm.pitch += -0.62831855f;
            this.rightLeg.pitch = -1.2566371f;
            this.leftLeg.pitch = -1.2566371f;
            this.rightLeg.yaw = 0.31415927f;
            this.leftLeg.yaw = -0.31415927f;
        }
        if (this.leftArmPose) {
            this.leftArm.pitch = this.leftArm.pitch * 0.5f - 0.31415927f;
        }
        if (this.rightArmPose) {
            this.rightArm.pitch = this.rightArm.pitch * 0.5f - 0.31415927f;
        }
        this.rightArm.yaw = 0.0f;
        this.leftArm.yaw = 0.0f;
        if (this.handSwingProgress > -9990.0f) {
            float f = this.handSwingProgress;
            this.body.yaw = MathHelper.sin(MathHelper.sqrt(f) * (float)Math.PI * 2.0f) * 0.2f;
            this.rightArm.pivotZ = MathHelper.sin(this.body.yaw) * 5.0f;
            this.rightArm.pivotX = -MathHelper.cos(this.body.yaw) * 5.0f;
            this.leftArm.pivotZ = -MathHelper.sin(this.body.yaw) * 5.0f;
            this.leftArm.pivotX = MathHelper.cos(this.body.yaw) * 5.0f;
            this.rightArm.yaw += this.body.yaw;
            this.leftArm.yaw += this.body.yaw;
            this.leftArm.pitch += this.body.yaw;
            f = 1.0f - this.handSwingProgress;
            f *= f;
            f *= f;
            f = 1.0f - f;
            float f2 = MathHelper.sin(f * (float)Math.PI);
            float f3 = MathHelper.sin(this.handSwingProgress * (float)Math.PI) * -(this.head.pitch - 0.7f) * 0.75f;
            this.rightArm.pitch = (float)((double)this.rightArm.pitch - ((double)f2 * 1.2 + (double)f3));
            this.rightArm.yaw += this.body.yaw * 2.0f;
            this.rightArm.roll = MathHelper.sin(this.handSwingProgress * (float)Math.PI) * -0.4f;
        }
        if (this.sneaking) {
            this.body.pitch = 0.5f;
            this.rightLeg.pitch -= 0.0f;
            this.leftLeg.pitch -= 0.0f;
            this.rightArm.pitch += 0.4f;
            this.leftArm.pitch += 0.4f;
            this.rightLeg.pivotZ = 4.0f;
            this.leftLeg.pivotZ = 4.0f;
            this.rightLeg.pivotY = 9.0f;
            this.leftLeg.pivotY = 9.0f;
            this.head.pivotY = 1.0f;
        } else {
            this.body.pitch = 0.0f;
            this.rightLeg.pivotZ = 0.0f;
            this.leftLeg.pivotZ = 0.0f;
            this.rightLeg.pivotY = 12.0f;
            this.leftLeg.pivotY = 12.0f;
            this.head.pivotY = 0.0f;
        }
        this.rightArm.roll += MathHelper.cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        this.leftArm.roll -= MathHelper.cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        this.rightArm.pitch += MathHelper.sin(animationProgress * 0.067f) * 0.05f;
        this.leftArm.pitch -= MathHelper.sin(animationProgress * 0.067f) * 0.05f;
    }

    public void renderEars(float scale) {
        this.ears.yaw = this.head.yaw;
        this.ears.pitch = this.head.pitch;
        this.ears.pivotX = 0.0f;
        this.ears.pivotY = 0.0f;
        this.ears.render(scale);
    }

    public void renderCape(float scale) {
        this.cape.render(scale);
    }
}

