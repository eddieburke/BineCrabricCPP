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
public class BoatEntityModel
extends EntityModel {
    public ModelPart[] parts = new ModelPart[5];

    public BoatEntityModel() {
        this.parts[0] = new ModelPart(0, 8);
        this.parts[1] = new ModelPart(0, 0);
        this.parts[2] = new ModelPart(0, 0);
        this.parts[3] = new ModelPart(0, 0);
        this.parts[4] = new ModelPart(0, 0);
        int n = 24;
        int n2 = 6;
        int n3 = 20;
        int n4 = 4;
        this.parts[0].addCuboid(-n / 2, -n3 / 2 + 2, -3.0f, n, n3 - 4, 4, 0.0f);
        this.parts[0].setPivot(0.0f, 0 + n4, 0.0f);
        this.parts[1].addCuboid(-n / 2 + 2, -n2 - 1, -1.0f, n - 4, n2, 2, 0.0f);
        this.parts[1].setPivot(-n / 2 + 1, 0 + n4, 0.0f);
        this.parts[2].addCuboid(-n / 2 + 2, -n2 - 1, -1.0f, n - 4, n2, 2, 0.0f);
        this.parts[2].setPivot(n / 2 - 1, 0 + n4, 0.0f);
        this.parts[3].addCuboid(-n / 2 + 2, -n2 - 1, -1.0f, n - 4, n2, 2, 0.0f);
        this.parts[3].setPivot(0.0f, 0 + n4, -n3 / 2 + 1);
        this.parts[4].addCuboid(-n / 2 + 2, -n2 - 1, -1.0f, n - 4, n2, 2, 0.0f);
        this.parts[4].setPivot(0.0f, 0 + n4, n3 / 2 - 1);
        this.parts[0].pitch = 1.5707964f;
        this.parts[1].yaw = 4.712389f;
        this.parts[2].yaw = 1.5707964f;
        this.parts[3].yaw = (float)Math.PI;
    }

    public void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
        for (int i = 0; i < 5; ++i) {
            this.parts[i].render(scale);
        }
    }

    public void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
    }
}

