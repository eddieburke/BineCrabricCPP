/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.model.ModelPart;

@Environment(value=EnvType.CLIENT)
public class SignModel {
    public ModelPart root = new ModelPart(0, 0);
    public ModelPart stick;

    public SignModel() {
        this.root.addCuboid(-12.0f, -14.0f, -1.0f, 24, 12, 2, 0.0f);
        this.stick = new ModelPart(0, 14);
        this.stick.addCuboid(-1.0f, -2.0f, -1.0f, 2, 14, 2, 0.0f);
    }

    public void render() {
        this.root.render(0.0625f);
        this.stick.render(0.0625f);
    }
}

