/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.entity.passive.CowEntity;

@Environment(value=EnvType.CLIENT)
public class CowEntityRenderer
extends LivingEntityRenderer {
    public CowEntityRenderer(EntityModel arg, float f) {
        super(arg, f);
    }

    public void render(CowEntity arg, double d, double e, double f, float g, float h) {
        super.render(arg, d, e, f, g, h);
    }
}

