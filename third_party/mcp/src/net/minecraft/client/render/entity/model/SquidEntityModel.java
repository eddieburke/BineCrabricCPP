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
public class SquidEntityModel
extends EntityModel {
    ModelPart root;
    ModelPart[] tentacles = new ModelPart[8];

    public SquidEntityModel() {
        int n = -16;
        this.root = new ModelPart(0, 0);
        this.root.addCuboid(-6.0f, -8.0f, -6.0f, 12, 16, 12);
        this.root.pivotY += (float)(24 + n);
        for (int i = 0; i < this.tentacles.length; ++i) {
            this.tentacles[i] = new ModelPart(48, 0);
            double d = (double)i * Math.PI * 2.0 / (double)this.tentacles.length;
            float f = (float)Math.cos(d) * 5.0f;
            float f2 = (float)Math.sin(d) * 5.0f;
            this.tentacles[i].addCuboid(-1.0f, 0.0f, -1.0f, 2, 18, 2);
            this.tentacles[i].pivotX = f;
            this.tentacles[i].pivotZ = f2;
            this.tentacles[i].pivotY = 31 + n;
            d = (double)i * Math.PI * -2.0 / (double)this.tentacles.length + 1.5707963267948966;
            this.tentacles[i].yaw = (float)d;
        }
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        for (int i = 0; i < this.tentacles.length; ++i) {
            this.tentacles[i].pitch = animationProgress;
        }
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        this.setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        this.root.render(scale);
        for (int i = 0; i < this.tentacles.length; ++i) {
            this.tentacles[i].render(scale);
        }
    }
}

