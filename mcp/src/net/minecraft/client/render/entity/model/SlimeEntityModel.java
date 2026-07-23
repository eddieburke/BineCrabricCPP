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

@Environment(value=EnvType.CLIENT)
public class SlimeEntityModel
extends EntityModel {
    ModelPart cube;
    ModelPart rightEye;
    ModelPart leftEye;
    ModelPart mouth;

    public SlimeEntityModel(int v) {
        this.cube = new ModelPart(0, v);
        this.cube.addCuboid(-4.0f, 16.0f, -4.0f, 8, 8, 8);
        if (v > 0) {
            this.cube = new ModelPart(0, v);
            this.cube.addCuboid(-3.0f, 17.0f, -3.0f, 6, 6, 6);
            this.rightEye = new ModelPart(32, 0);
            this.rightEye.addCuboid(-3.25f, 18.0f, -3.5f, 2, 2, 2);
            this.leftEye = new ModelPart(32, 4);
            this.leftEye.addCuboid(1.25f, 18.0f, -3.5f, 2, 2, 2);
            this.mouth = new ModelPart(32, 8);
            this.mouth.addCuboid(0.0f, 21.0f, -3.5f, 1, 1, 1);
        }
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.cube.render(scale);
        if (this.rightEye != null) {
            this.rightEye.render(scale);
            this.leftEye.render(scale);
            this.mouth.render(scale);
        }
    }
}

