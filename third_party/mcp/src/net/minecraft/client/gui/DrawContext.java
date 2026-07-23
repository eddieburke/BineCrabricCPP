/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.Tessellator;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class DrawContext {
    protected float zOffset = 0.0f;

    protected void drawHorizontalLine(int x1, int x2, int y, int color) {
        if (x2 < x1) {
            int n = x1;
            x1 = x2;
            x2 = n;
        }
        this.fill(x1, y, x2 + 1, y + 1, color);
    }

    protected void drawVerticalLine(int x, int y1, int y2, int color) {
        if (y2 < y1) {
            int n = y1;
            y1 = y2;
            y2 = n;
        }
        this.fill(x, y1 + 1, x + 1, y2, color);
    }

    protected void fill(int x1, int y1, int x2, int y2, int color) {
        int n;
        if (x1 < x2) {
            n = x1;
            x1 = x2;
            x2 = n;
        }
        if (y1 < y2) {
            n = y1;
            y1 = y2;
            y2 = n;
        }
        float f = (float)(color >> 24 & 0xFF) / 255.0f;
        float f2 = (float)(color >> 16 & 0xFF) / 255.0f;
        float f3 = (float)(color >> 8 & 0xFF) / 255.0f;
        float f4 = (float)(color & 0xFF) / 255.0f;
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glEnable(3042);
        GL11.glDisable(3553);
        GL11.glBlendFunc(770, 771);
        GL11.glColor4f(f2, f3, f4, f);
        tessellator.startQuads();
        tessellator.vertex(x1, y2, 0.0);
        tessellator.vertex(x2, y2, 0.0);
        tessellator.vertex(x2, y1, 0.0);
        tessellator.vertex(x1, y1, 0.0);
        tessellator.draw();
        GL11.glEnable(3553);
        GL11.glDisable(3042);
    }

    protected void fillGradient(int startX, int startY, int endX, int endY, int colorStart, int colorEnd) {
        float f = (float)(colorStart >> 24 & 0xFF) / 255.0f;
        float f2 = (float)(colorStart >> 16 & 0xFF) / 255.0f;
        float f3 = (float)(colorStart >> 8 & 0xFF) / 255.0f;
        float f4 = (float)(colorStart & 0xFF) / 255.0f;
        float f5 = (float)(colorEnd >> 24 & 0xFF) / 255.0f;
        float f6 = (float)(colorEnd >> 16 & 0xFF) / 255.0f;
        float f7 = (float)(colorEnd >> 8 & 0xFF) / 255.0f;
        float f8 = (float)(colorEnd & 0xFF) / 255.0f;
        GL11.glDisable(3553);
        GL11.glEnable(3042);
        GL11.glDisable(3008);
        GL11.glBlendFunc(770, 771);
        GL11.glShadeModel(7425);
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.color(f2, f3, f4, f);
        tessellator.vertex(endX, startY, 0.0);
        tessellator.vertex(startX, startY, 0.0);
        tessellator.color(f6, f7, f8, f5);
        tessellator.vertex(startX, endY, 0.0);
        tessellator.vertex(endX, endY, 0.0);
        tessellator.draw();
        GL11.glShadeModel(7424);
        GL11.glDisable(3042);
        GL11.glEnable(3008);
        GL11.glEnable(3553);
    }

    public void drawCenteredTextWithShadow(TextRenderer textRenderer, String text, int centerX, int y, int color) {
        textRenderer.drawWithShadow(text, centerX - textRenderer.getWidth(text) / 2, y, color);
    }

    public void drawTextWithShadow(TextRenderer textRenderer, String text, int x, int y, int color) {
        textRenderer.drawWithShadow(text, x, y, color);
    }

    public void drawTexture(int x, int y, int u, int v, int width, int height) {
        float f = 0.00390625f;
        float f2 = 0.00390625f;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(x + 0, y + height, this.zOffset, (float)(u + 0) * f, (float)(v + height) * f2);
        tessellator.vertex(x + width, y + height, this.zOffset, (float)(u + width) * f, (float)(v + height) * f2);
        tessellator.vertex(x + width, y + 0, this.zOffset, (float)(u + width) * f, (float)(v + 0) * f2);
        tessellator.vertex(x + 0, y + 0, this.zOffset, (float)(u + 0) * f, (float)(v + 0) * f2);
        tessellator.draw();
    }
}

