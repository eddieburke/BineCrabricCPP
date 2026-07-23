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
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.render.entity.model.BoatEntityModel;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class BoatEntityRenderer
extends EntityRenderer {
    protected EntityModel model;

    public BoatEntityRenderer() {
        this.shadowRadius = 0.5f;
        this.model = new BoatEntityModel();
    }

    public void render(BoatEntity arg, double d, double e, double f, float g, float h) {
        GL11.glPushMatrix();
        GL11.glTranslatef((float)d, (float)e, (float)f);
        GL11.glRotatef(180.0f - g, 0.0f, 1.0f, 0.0f);
        float f2 = (float)arg.damageWobbleTicks - h;
        float f3 = (float)arg.damageWobbleStrength - h;
        if (f3 < 0.0f) {
            f3 = 0.0f;
        }
        if (f2 > 0.0f) {
            GL11.glRotatef(MathHelper.sin(f2) * f2 * f3 / 10.0f * (float)arg.damageWobbleSide, 1.0f, 0.0f, 0.0f);
        }
        this.bindTexture("/terrain.png");
        float f4 = 0.75f;
        GL11.glScalef(f4, f4, f4);
        GL11.glScalef(1.0f / f4, 1.0f / f4, 1.0f / f4);
        this.bindTexture("/item/boat.png");
        GL11.glScalef(-1.0f, -1.0f, 1.0f);
        this.model.render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
        GL11.glPopMatrix();
    }
}

