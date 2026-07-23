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
import net.minecraft.entity.passive.SheepEntity;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class SheepEntityRenderer
extends LivingEntityRenderer {
    public SheepEntityRenderer(EntityModel model, EntityModel furModel, float shadowRadius) {
        super(model, shadowRadius);
        this.setDecorationModel(furModel);
    }

    protected boolean bindTexture(SheepEntity arg, int i, float f) {
        if (i == 0 && !arg.isSheared()) {
            this.bindTexture("/mob/sheep_fur.png");
            float f2 = arg.getBrightnessAtEyes(f);
            int n = arg.getColor();
            GL11.glColor3f(f2 * SheepEntity.COLORS[n][0], f2 * SheepEntity.COLORS[n][1], f2 * SheepEntity.COLORS[n][2]);
            return true;
        }
        return false;
    }
}

