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
import net.minecraft.client.render.entity.model.ZombieEntityModel;

@Environment(value=EnvType.CLIENT)
public class SkeletonEntityModel
extends ZombieEntityModel {
    public SkeletonEntityModel() {
        float f = 0.0f;
        this.rightArm = new ModelPart(40, 16);
        this.rightArm.addCuboid(-1.0f, -2.0f, -1.0f, 2, 12, 2, f);
        this.rightArm.setPivot(-5.0f, 2.0f, 0.0f);
        this.leftArm = new ModelPart(40, 16);
        this.leftArm.mirror = true;
        this.leftArm.addCuboid(-1.0f, -2.0f, -1.0f, 2, 12, 2, f);
        this.leftArm.setPivot(5.0f, 2.0f, 0.0f);
        this.rightLeg = new ModelPart(0, 16);
        this.rightLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 12, 2, f);
        this.rightLeg.setPivot(-2.0f, 12.0f, 0.0f);
        this.leftLeg = new ModelPart(0, 16);
        this.leftLeg.mirror = true;
        this.leftLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 12, 2, f);
        this.leftLeg.setPivot(2.0f, 12.0f, 0.0f);
    }
}

