/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.item;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.ItemEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ItemRenderer
extends EntityRenderer {
    private BlockRenderManager blockRenderer = new BlockRenderManager();
    private Random random = new Random();
    public boolean useCustomDisplayColor = true;

    public ItemRenderer() {
        this.shadowRadius = 0.15f;
        this.shadowDarkness = 0.75f;
    }

    public void render(ItemEntity arg, double d, double e, double f, float g, float h) {
        this.random.setSeed(187L);
        ItemStack itemStack = arg.stack;
        GL11.glPushMatrix();
        float f2 = MathHelper.sin(((float)arg.itemAge + h) / 10.0f + arg.initialRotationAngle) * 0.1f + 0.1f;
        float f3 = (((float)arg.itemAge + h) / 20.0f + arg.initialRotationAngle) * 57.295776f;
        int n = 1;
        if (arg.stack.count > 1) {
            n = 2;
        }
        if (arg.stack.count > 5) {
            n = 3;
        }
        if (arg.stack.count > 20) {
            n = 4;
        }
        GL11.glTranslatef((float)d, (float)e + f2, (float)f);
        GL11.glEnable(32826);
        if (itemStack.itemId < 256 && BlockRenderManager.isSideLit(Block.BLOCKS[itemStack.itemId].getRenderType())) {
            GL11.glRotatef(f3, 0.0f, 1.0f, 0.0f);
            this.bindTexture("/terrain.png");
            float f4 = 0.25f;
            if (!Block.BLOCKS[itemStack.itemId].isFullCube() && itemStack.itemId != Block.SLAB.id && Block.BLOCKS[itemStack.itemId].getRenderType() != 16) {
                f4 = 0.5f;
            }
            GL11.glScalef(f4, f4, f4);
            for (int i = 0; i < n; ++i) {
                GL11.glPushMatrix();
                if (i > 0) {
                    float f5 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.2f / f4;
                    float f6 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.2f / f4;
                    float f7 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.2f / f4;
                    GL11.glTranslatef(f5, f6, f7);
                }
                this.blockRenderer.render(Block.BLOCKS[itemStack.itemId], itemStack.getDamage(), arg.getBrightnessAtEyes(h));
                GL11.glPopMatrix();
            }
        } else {
            float f8;
            float f9;
            float f10;
            int n2;
            GL11.glScalef(0.5f, 0.5f, 0.5f);
            int n3 = itemStack.getTextureId();
            if (itemStack.itemId < 256) {
                this.bindTexture("/terrain.png");
            } else {
                this.bindTexture("/gui/items.png");
            }
            Tessellator tessellator = Tessellator.INSTANCE;
            float f11 = (float)(n3 % 16 * 16 + 0) / 256.0f;
            float f12 = (float)(n3 % 16 * 16 + 16) / 256.0f;
            float f13 = (float)(n3 / 16 * 16 + 0) / 256.0f;
            float f14 = (float)(n3 / 16 * 16 + 16) / 256.0f;
            float f15 = 1.0f;
            float f16 = 0.5f;
            float f17 = 0.25f;
            if (this.useCustomDisplayColor) {
                n2 = Item.ITEMS[itemStack.itemId].getColorMultiplier(itemStack.getDamage());
                f10 = (float)(n2 >> 16 & 0xFF) / 255.0f;
                f9 = (float)(n2 >> 8 & 0xFF) / 255.0f;
                f8 = (float)(n2 & 0xFF) / 255.0f;
                float f18 = arg.getBrightnessAtEyes(h);
                GL11.glColor4f(f10 * f18, f9 * f18, f8 * f18, 1.0f);
            }
            for (n2 = 0; n2 < n; ++n2) {
                GL11.glPushMatrix();
                if (n2 > 0) {
                    f10 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.3f;
                    f9 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.3f;
                    f8 = (this.random.nextFloat() * 2.0f - 1.0f) * 0.3f;
                    GL11.glTranslatef(f10, f9, f8);
                }
                GL11.glRotatef(180.0f - this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
                tessellator.startQuads();
                tessellator.normal(0.0f, 1.0f, 0.0f);
                tessellator.vertex(0.0f - f16, 0.0f - f17, 0.0, f11, f14);
                tessellator.vertex(f15 - f16, 0.0f - f17, 0.0, f12, f14);
                tessellator.vertex(f15 - f16, 1.0f - f17, 0.0, f12, f13);
                tessellator.vertex(0.0f - f16, 1.0f - f17, 0.0, f11, f13);
                tessellator.draw();
                GL11.glPopMatrix();
            }
        }
        GL11.glDisable(32826);
        GL11.glPopMatrix();
    }

    public void renderGuiItem(TextRenderer textRenderer, TextureManager textureManager, int item, int meta, int sprite, int x, int y) {
        if (item < 256 && BlockRenderManager.isSideLit(Block.BLOCKS[item].getRenderType())) {
            int n = item;
            textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
            Block block = Block.BLOCKS[n];
            GL11.glPushMatrix();
            GL11.glTranslatef(x - 2, y + 3, -3.0f);
            GL11.glScalef(10.0f, 10.0f, 10.0f);
            GL11.glTranslatef(1.0f, 0.5f, 1.0f);
            GL11.glScalef(1.0f, 1.0f, -1.0f);
            GL11.glRotatef(210.0f, 1.0f, 0.0f, 0.0f);
            GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            int n2 = Item.ITEMS[item].getColorMultiplier(meta);
            float f = (float)(n2 >> 16 & 0xFF) / 255.0f;
            float f2 = (float)(n2 >> 8 & 0xFF) / 255.0f;
            float f3 = (float)(n2 & 0xFF) / 255.0f;
            if (this.useCustomDisplayColor) {
                GL11.glColor4f(f, f2, f3, 1.0f);
            }
            GL11.glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
            this.blockRenderer.inventoryColorEnabled = this.useCustomDisplayColor;
            this.blockRenderer.render(block, meta, 1.0f);
            this.blockRenderer.inventoryColorEnabled = true;
            GL11.glPopMatrix();
        } else if (sprite >= 0) {
            GL11.glDisable(2896);
            if (item < 256) {
                textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
            } else {
                textureManager.bindTexture(textureManager.getTextureId("/gui/items.png"));
            }
            int n = Item.ITEMS[item].getColorMultiplier(meta);
            float f = (float)(n >> 16 & 0xFF) / 255.0f;
            float f4 = (float)(n >> 8 & 0xFF) / 255.0f;
            float f5 = (float)(n & 0xFF) / 255.0f;
            if (this.useCustomDisplayColor) {
                GL11.glColor4f(f, f4, f5, 1.0f);
            }
            this.drawTexture(x, y, sprite % 16 * 16, sprite / 16 * 16, 16, 16);
            GL11.glEnable(2896);
        }
        GL11.glEnable(2884);
    }

    public void renderGuiItem(TextRenderer textRenderer, TextureManager textureManager, ItemStack stack, int x, int y) {
        if (stack == null) {
            return;
        }
        this.renderGuiItem(textRenderer, textureManager, stack.itemId, stack.getDamage(), stack.getTextureId(), x, y);
    }

    public void renderGuiItemDecoration(TextRenderer textrenderer, TextureManager textureManager, ItemStack stack, int x, int y) {
        if (stack == null) {
            return;
        }
        if (stack.count > 1) {
            String string = "" + stack.count;
            GL11.glDisable(2896);
            GL11.glDisable(2929);
            textrenderer.drawWithShadow(string, x + 19 - 2 - textrenderer.getWidth(string), y + 6 + 3, 0xFFFFFF);
            GL11.glEnable(2896);
            GL11.glEnable(2929);
        }
        if (stack.isDamaged()) {
            int n = (int)Math.round(13.0 - (double)stack.getDamage2() * 13.0 / (double)stack.getMaxDamage());
            int n2 = (int)Math.round(255.0 - (double)stack.getDamage2() * 255.0 / (double)stack.getMaxDamage());
            GL11.glDisable(2896);
            GL11.glDisable(2929);
            GL11.glDisable(3553);
            Tessellator tessellator = Tessellator.INSTANCE;
            int n3 = 255 - n2 << 16 | n2 << 8;
            int n4 = (255 - n2) / 4 << 16 | 0x3F00;
            this.fillRect(tessellator, x + 2, y + 13, 13, 2, 0);
            this.fillRect(tessellator, x + 2, y + 13, 12, 1, n4);
            this.fillRect(tessellator, x + 2, y + 13, n, 1, n3);
            GL11.glEnable(3553);
            GL11.glEnable(2896);
            GL11.glEnable(2929);
            GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    private void fillRect(Tessellator tessellator, int x, int y, int width, int height, int color) {
        tessellator.startQuads();
        tessellator.color(color);
        tessellator.vertex(x + 0, y + 0, 0.0);
        tessellator.vertex(x + 0, y + height, 0.0);
        tessellator.vertex(x + width, y + height, 0.0);
        tessellator.vertex(x + width, y + 0, 0.0);
        tessellator.draw();
    }

    public void drawTexture(int x, int y, int u, int v, int width, int height) {
        float f = 0.0f;
        float f2 = 0.00390625f;
        float f3 = 0.00390625f;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(x + 0, y + height, f, (float)(u + 0) * f2, (float)(v + height) * f3);
        tessellator.vertex(x + width, y + height, f, (float)(u + width) * f2, (float)(v + height) * f3);
        tessellator.vertex(x + width, y + 0, f, (float)(u + width) * f2, (float)(v + 0) * f3);
        tessellator.vertex(x + 0, y + 0, f, (float)(u + 0) * f2, (float)(v + 0) * f3);
        tessellator.draw();
    }
}

