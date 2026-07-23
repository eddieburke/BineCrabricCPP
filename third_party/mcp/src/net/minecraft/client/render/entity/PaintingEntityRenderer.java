/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.decoration.painting.PaintingVariants;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class PaintingEntityRenderer
extends EntityRenderer {
    private Random random = new Random();

    public void render(PaintingEntity arg, double d, double e, double f, float g, float h) {
        this.random.setSeed(187L);
        GL11.glPushMatrix();
        GL11.glTranslatef((float)d, (float)e, (float)f);
        GL11.glRotatef(g, 0.0f, 1.0f, 0.0f);
        GL11.glEnable(32826);
        this.bindTexture("/art/kz.png");
        PaintingVariants paintingVariants = arg.variant;
        float f2 = 0.0625f;
        GL11.glScalef(f2, f2, f2);
        this.renderPainting(arg, paintingVariants.width, paintingVariants.height, paintingVariants.textureOffsetX, paintingVariants.textureOffsetY);
        GL11.glDisable(32826);
        GL11.glPopMatrix();
    }

    private void renderPainting(PaintingEntity painting, int width, int height, int u, int v) {
        float f = (float)(-width) / 2.0f;
        float f2 = (float)(-height) / 2.0f;
        float f3 = -0.5f;
        float f4 = 0.5f;
        for (int i = 0; i < width / 16; ++i) {
            for (int j = 0; j < height / 16; ++j) {
                float f5 = f + (float)((i + 1) * 16);
                float f6 = f + (float)(i * 16);
                float f7 = f2 + (float)((j + 1) * 16);
                float f8 = f2 + (float)(j * 16);
                this.applyBrightness(painting, (f5 + f6) / 2.0f, (f7 + f8) / 2.0f);
                float f9 = (float)(u + width - i * 16) / 256.0f;
                float f10 = (float)(u + width - (i + 1) * 16) / 256.0f;
                float f11 = (float)(v + height - j * 16) / 256.0f;
                float f12 = (float)(v + height - (j + 1) * 16) / 256.0f;
                float f13 = 0.75f;
                float f14 = 0.8125f;
                float f15 = 0.0f;
                float f16 = 0.0625f;
                float f17 = 0.75f;
                float f18 = 0.8125f;
                float f19 = 0.001953125f;
                float f20 = 0.001953125f;
                float f21 = 0.7519531f;
                float f22 = 0.7519531f;
                float f23 = 0.0f;
                float f24 = 0.0625f;
                Tessellator tessellator = Tessellator.INSTANCE;
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, -1.0f);
                tessellator.vertex(f5, f8, f3, f10, f11);
                tessellator.vertex(f6, f8, f3, f9, f11);
                tessellator.vertex(f6, f7, f3, f9, f12);
                tessellator.vertex(f5, f7, f3, f10, f12);
                tessellator.normal(0.0f, 0.0f, 1.0f);
                tessellator.vertex(f5, f7, f4, f13, f15);
                tessellator.vertex(f6, f7, f4, f14, f15);
                tessellator.vertex(f6, f8, f4, f14, f16);
                tessellator.vertex(f5, f8, f4, f13, f16);
                tessellator.normal(0.0f, -1.0f, 0.0f);
                tessellator.vertex(f5, f7, f3, f17, f19);
                tessellator.vertex(f6, f7, f3, f18, f19);
                tessellator.vertex(f6, f7, f4, f18, f20);
                tessellator.vertex(f5, f7, f4, f17, f20);
                tessellator.normal(0.0f, 1.0f, 0.0f);
                tessellator.vertex(f5, f8, f4, f17, f19);
                tessellator.vertex(f6, f8, f4, f18, f19);
                tessellator.vertex(f6, f8, f3, f18, f20);
                tessellator.vertex(f5, f8, f3, f17, f20);
                tessellator.normal(-1.0f, 0.0f, 0.0f);
                tessellator.vertex(f5, f7, f4, f22, f23);
                tessellator.vertex(f5, f8, f4, f22, f24);
                tessellator.vertex(f5, f8, f3, f21, f24);
                tessellator.vertex(f5, f7, f3, f21, f23);
                tessellator.normal(1.0f, 0.0f, 0.0f);
                tessellator.vertex(f6, f7, f3, f22, f23);
                tessellator.vertex(f6, f8, f3, f22, f24);
                tessellator.vertex(f6, f8, f4, f21, f24);
                tessellator.vertex(f6, f7, f4, f21, f23);
                tessellator.draw();
            }
        }
    }

    private void applyBrightness(PaintingEntity painting, float u, float v) {
        int n = MathHelper.floor(painting.x);
        int n2 = MathHelper.floor(painting.y + (double)(v / 16.0f));
        int n3 = MathHelper.floor(painting.z);
        if (painting.facing == 0) {
            n = MathHelper.floor(painting.x + (double)(u / 16.0f));
        }
        if (painting.facing == 1) {
            n3 = MathHelper.floor(painting.z - (double)(u / 16.0f));
        }
        if (painting.facing == 2) {
            n = MathHelper.floor(painting.x - (double)(u / 16.0f));
        }
        if (painting.facing == 3) {
            n3 = MathHelper.floor(painting.z + (double)(u / 16.0f));
        }
        float f = this.dispatcher.world.getLightBrightness(n, n2, n3);
        GL11.glColor3f(f, f, f);
    }
}

