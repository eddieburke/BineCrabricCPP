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
public class SheepEntityModel
extends QuadrupedEntityModel {
    public SheepEntityModel() {
        super(12, 0.0f);
        this.head = new ModelPart(0, 0);
        this.head.addCuboid(-3.0f, -4.0f, -6.0f, 6, 6, 8, 0.0f);
        this.head.setPivot(0.0f, 6.0f, -8.0f);
        this.body = new ModelPart(28, 8);
        this.body.addCuboid(-4.0f, -10.0f, -7.0f, 8, 16, 6, 0.0f);
        this.body.setPivot(0.0f, 5.0f, 2.0f);
    }
}

