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
public class ChickenEntityModel
extends EntityModel {
    public ModelPart head;
    public ModelPart body;
    public ModelPart rightLeg;
    public ModelPart leftLeg;
    public ModelPart rightWing;
    public ModelPart leftWing;
    public ModelPart beak;
    public ModelPart wattle;

    public ChickenEntityModel() {
        int n = 16;
        this.head = new ModelPart(0, 0);
        this.head.addCuboid(-2.0f, -6.0f, -2.0f, 4, 6, 3, 0.0f);
        this.head.setPivot(0.0f, -1 + n, -4.0f);
        this.beak = new ModelPart(14, 0);
        this.beak.addCuboid(-2.0f, -4.0f, -4.0f, 4, 2, 2, 0.0f);
        this.beak.setPivot(0.0f, -1 + n, -4.0f);
        this.wattle = new ModelPart(14, 4);
        this.wattle.addCuboid(-1.0f, -2.0f, -3.0f, 2, 2, 2, 0.0f);
        this.wattle.setPivot(0.0f, -1 + n, -4.0f);
        this.body = new ModelPart(0, 9);
        this.body.addCuboid(-3.0f, -4.0f, -3.0f, 6, 8, 6, 0.0f);
        this.body.setPivot(0.0f, 0 + n, 0.0f);
        this.rightLeg = new ModelPart(26, 0);
        this.rightLeg.addCuboid(-1.0f, 0.0f, -3.0f, 3, 5, 3);
        this.rightLeg.setPivot(-2.0f, 3 + n, 1.0f);
        this.leftLeg = new ModelPart(26, 0);
        this.leftLeg.addCuboid(-1.0f, 0.0f, -3.0f, 3, 5, 3);
        this.leftLeg.setPivot(1.0f, 3 + n, 1.0f);
        this.rightWing = new ModelPart(24, 13);
        this.rightWing.addCuboid(0.0f, 0.0f, -3.0f, 1, 4, 6);
        this.rightWing.setPivot(-4.0f, -3 + n, 0.0f);
        this.leftWing = new ModelPart(24, 13);
        this.leftWing.addCuboid(-1.0f, 0.0f, -3.0f, 1, 4, 6);
        this.leftWing.setPivot(4.0f, -3 + n, 0.0f);
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.head.render(scale);
        this.beak.render(scale);
        this.wattle.render(scale);
        this.body.render(scale);
        this.rightLeg.render(scale);
        this.leftLeg.render(scale);
        this.rightWing.render(scale);
        this.leftWing.render(scale);
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.head.pitch = -(headPitch / 57.295776f);
        this.head.yaw = headYaw / 57.295776f;
        this.beak.pitch = this.head.pitch;
        this.beak.yaw = this.head.yaw;
        this.wattle.pitch = this.head.pitch;
        this.wattle.yaw = this.head.yaw;
        this.body.pitch = 1.5707964f;
        this.rightLeg.pitch = MathHelper.cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
        this.leftLeg.pitch = MathHelper.cos(limbAngle * 0.6662f + (float)Math.PI) * 1.4f * limbDistance;
        this.rightWing.roll = animationProgress;
        this.leftWing.roll = -animationProgress;
    }
}

