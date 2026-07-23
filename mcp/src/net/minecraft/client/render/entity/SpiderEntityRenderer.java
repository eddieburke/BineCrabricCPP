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
import net.minecraft.client.render.entity.model.SpiderEntityModel;
import net.minecraft.entity.mob.SpiderEntity;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class SpiderEntityRenderer
extends LivingEntityRenderer {
    public SpiderEntityRenderer() {
        super(new SpiderEntityModel(), 1.0f);
        this.setDecorationModel(new SpiderEntityModel());
    }

    protected float getDeathYaw(SpiderEntity arg) {
        return 180.0f;
    }

    protected boolean bindTexture(SpiderEntity arg, int i, float f) {
        if (i != 0) {
            return false;
        }
        if (i != 0) {
            return false;
        }
        this.bindTexture("/mob/spider_eyes.png");
        float f2 = (1.0f - arg.getBrightnessAtEyes(1.0f)) * 0.5f;
        GL11.glEnable(3042);
        GL11.glDisable(3008);
        GL11.glBlendFunc(770, 771);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, f2);
        return true;
    }
}

