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
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.item.Item;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class FireballEntityRenderer
extends EntityRenderer {
    public void render(FireballEntity arg, double d, double e, double f, float g, float h) {
        GL11.glPushMatrix();
        GL11.glTranslatef((float)d, (float)e, (float)f);
        GL11.glEnable(32826);
        float f2 = 2.0f;
        GL11.glScalef(f2 / 1.0f, f2 / 1.0f, f2 / 1.0f);
        int n = Item.SNOWBALL.getTextureId(0);
        this.bindTexture("/gui/items.png");
        Tessellator tessellator = Tessellator.INSTANCE;
        float f3 = (float)(n % 16 * 16 + 0) / 256.0f;
        float f4 = (float)(n % 16 * 16 + 16) / 256.0f;
        float f5 = (float)(n / 16 * 16 + 0) / 256.0f;
        float f6 = (float)(n / 16 * 16 + 16) / 256.0f;
        float f7 = 1.0f;
        float f8 = 0.5f;
        float f9 = 0.25f;
        GL11.glRotatef(180.0f - this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-this.dispatcher.pitch, 1.0f, 0.0f, 0.0f);
        tessellator.startQuads();
        tessellator.normal(0.0f, 1.0f, 0.0f);
        tessellator.vertex(0.0f - f8, 0.0f - f9, 0.0, f3, f6);
        tessellator.vertex(f7 - f8, 0.0f - f9, 0.0, f4, f6);
        tessellator.vertex(f7 - f8, 1.0f - f9, 0.0, f4, f5);
        tessellator.vertex(0.0f - f8, 1.0f - f9, 0.0, f3, f5);
        tessellator.draw();
        GL11.glDisable(32826);
        GL11.glPopMatrix();
    }
}

