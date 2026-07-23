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
import net.minecraft.block.Block;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.block.entity.BlockEntityRenderer;
import net.minecraft.client.render.block.entity.SignModel;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class SignBlockEntityRenderer
extends BlockEntityRenderer {
    private SignModel model = new SignModel();

    public void render(SignBlockEntity arg, double d, double e, double f, float g) {
        float f2;
        Block block = arg.getBlock();
        GL11.glPushMatrix();
        float f3 = 0.6666667f;
        if (block == Block.SIGN) {
            GL11.glTranslatef((float)d + 0.5f, (float)e + 0.75f * f3, (float)f + 0.5f);
            float f4 = (float)(arg.getPushedBlockData() * 360) / 16.0f;
            GL11.glRotatef(-f4, 0.0f, 1.0f, 0.0f);
            this.model.stick.visible = true;
        } else {
            int n = arg.getPushedBlockData();
            f2 = 0.0f;
            if (n == 2) {
                f2 = 180.0f;
            }
            if (n == 4) {
                f2 = 90.0f;
            }
            if (n == 5) {
                f2 = -90.0f;
            }
            GL11.glTranslatef((float)d + 0.5f, (float)e + 0.75f * f3, (float)f + 0.5f);
            GL11.glRotatef(-f2, 0.0f, 1.0f, 0.0f);
            GL11.glTranslatef(0.0f, -0.3125f, -0.4375f);
            this.model.stick.visible = false;
        }
        this.bindTexture("/item/sign.png");
        GL11.glPushMatrix();
        GL11.glScalef(f3, -f3, -f3);
        this.model.render();
        GL11.glPopMatrix();
        TextRenderer textRenderer = this.getTextRenderer();
        f2 = 0.016666668f * f3;
        GL11.glTranslatef(0.0f, 0.5f * f3, 0.07f * f3);
        GL11.glScalef(f2, -f2, f2);
        GL11.glNormal3f(0.0f, 0.0f, -1.0f * f2);
        GL11.glDepthMask(false);
        int n = 0;
        for (int i = 0; i < arg.texts.length; ++i) {
            String string = arg.texts[i];
            if (i == arg.currentRow) {
                string = "> " + string + " <";
                textRenderer.draw(string, -textRenderer.getWidth(string) / 2, i * 10 - arg.texts.length * 5, n);
                continue;
            }
            textRenderer.draw(string, -textRenderer.getWidth(string) / 2, i * 10 - arg.texts.length * 5, n);
        }
        GL11.glDepthMask(true);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glPopMatrix();
    }
}

