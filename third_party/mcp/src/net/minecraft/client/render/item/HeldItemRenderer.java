/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.item;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.client.Minecraft;
import net.minecraft.client.render.MapRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.render.entity.PlayerEntityRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.ClientPlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.map.MapState;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class HeldItemRenderer {
    private Minecraft minecraft;
    private ItemStack stack = null;
    private float height = 0.0f;
    private float prevHeight = 0.0f;
    private BlockRenderManager blockRenderManager = new BlockRenderManager();
    private MapRenderer mapRenderer;
    private int slot = -1;

    public HeldItemRenderer(Minecraft minecraft) {
        this.minecraft = minecraft;
        this.mapRenderer = new MapRenderer(minecraft.textRenderer, minecraft.options, minecraft.textureManager);
    }

    public void renderItem(LivingEntity entity, ItemStack stack) {
        GL11.glPushMatrix();
        if (stack.itemId < 256 && BlockRenderManager.isSideLit(Block.BLOCKS[stack.itemId].getRenderType())) {
            GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/terrain.png"));
            this.blockRenderManager.render(Block.BLOCKS[stack.itemId], stack.getDamage(), entity.getBrightnessAtEyes(1.0f));
        } else {
            float f;
            float f2;
            float f3;
            int n;
            if (stack.itemId < 256) {
                GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/terrain.png"));
            } else {
                GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/items.png"));
            }
            Tessellator tessellator = Tessellator.INSTANCE;
            int n2 = entity.getItemStackTextureId(stack);
            float f4 = ((float)(n2 % 16 * 16) + 0.0f) / 256.0f;
            float f5 = ((float)(n2 % 16 * 16) + 15.99f) / 256.0f;
            float f6 = ((float)(n2 / 16 * 16) + 0.0f) / 256.0f;
            float f7 = ((float)(n2 / 16 * 16) + 15.99f) / 256.0f;
            float f8 = 1.0f;
            float f9 = 0.0f;
            float f10 = 0.3f;
            GL11.glEnable(32826);
            GL11.glTranslatef(-f9, -f10, 0.0f);
            float f11 = 1.5f;
            GL11.glScalef(f11, f11, f11);
            GL11.glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
            GL11.glTranslatef(-0.9375f, -0.0625f, 0.0f);
            float f12 = 0.0625f;
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, 1.0f);
            tessellator.vertex(0.0, 0.0, 0.0, f5, f7);
            tessellator.vertex(f8, 0.0, 0.0, f4, f7);
            tessellator.vertex(f8, 1.0, 0.0, f4, f6);
            tessellator.vertex(0.0, 1.0, 0.0, f5, f6);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, -1.0f);
            tessellator.vertex(0.0, 1.0, 0.0f - f12, f5, f6);
            tessellator.vertex(f8, 1.0, 0.0f - f12, f4, f6);
            tessellator.vertex(f8, 0.0, 0.0f - f12, f4, f7);
            tessellator.vertex(0.0, 0.0, 0.0f - f12, f5, f7);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(-1.0f, 0.0f, 0.0f);
            for (n = 0; n < 16; ++n) {
                f3 = (float)n / 16.0f;
                f2 = f5 + (f4 - f5) * f3 - 0.001953125f;
                f = f8 * f3;
                tessellator.vertex(f, 0.0, 0.0f - f12, f2, f7);
                tessellator.vertex(f, 0.0, 0.0, f2, f7);
                tessellator.vertex(f, 1.0, 0.0, f2, f6);
                tessellator.vertex(f, 1.0, 0.0f - f12, f2, f6);
            }
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(1.0f, 0.0f, 0.0f);
            for (n = 0; n < 16; ++n) {
                f3 = (float)n / 16.0f;
                f2 = f5 + (f4 - f5) * f3 - 0.001953125f;
                f = f8 * f3 + 0.0625f;
                tessellator.vertex(f, 1.0, 0.0f - f12, f2, f6);
                tessellator.vertex(f, 1.0, 0.0, f2, f6);
                tessellator.vertex(f, 0.0, 0.0, f2, f7);
                tessellator.vertex(f, 0.0, 0.0f - f12, f2, f7);
            }
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 1.0f, 0.0f);
            for (n = 0; n < 16; ++n) {
                f3 = (float)n / 16.0f;
                f2 = f7 + (f6 - f7) * f3 - 0.001953125f;
                f = f8 * f3 + 0.0625f;
                tessellator.vertex(0.0, f, 0.0, f5, f2);
                tessellator.vertex(f8, f, 0.0, f4, f2);
                tessellator.vertex(f8, f, 0.0f - f12, f4, f2);
                tessellator.vertex(0.0, f, 0.0f - f12, f5, f2);
            }
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            for (n = 0; n < 16; ++n) {
                f3 = (float)n / 16.0f;
                f2 = f7 + (f6 - f7) * f3 - 0.001953125f;
                f = f8 * f3;
                tessellator.vertex(f8, f, 0.0, f4, f2);
                tessellator.vertex(0.0, f, 0.0, f5, f2);
                tessellator.vertex(0.0, f, 0.0f - f12, f5, f2);
                tessellator.vertex(f8, f, 0.0f - f12, f4, f2);
            }
            tessellator.draw();
            GL11.glDisable(32826);
        }
        GL11.glPopMatrix();
    }

    public void render(float tickDelta) {
        float f;
        float f2;
        float f3;
        float f4 = this.prevHeight + (this.height - this.prevHeight) * tickDelta;
        ClientPlayerEntity clientPlayerEntity = this.minecraft.player;
        float f5 = clientPlayerEntity.prevPitch + (clientPlayerEntity.pitch - clientPlayerEntity.prevPitch) * tickDelta;
        GL11.glPushMatrix();
        GL11.glRotatef(f5, 1.0f, 0.0f, 0.0f);
        GL11.glRotatef(clientPlayerEntity.prevYaw + (clientPlayerEntity.yaw - clientPlayerEntity.prevYaw) * tickDelta, 0.0f, 1.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        ItemStack itemStack = this.stack;
        float f6 = this.minecraft.world.getLightBrightness(MathHelper.floor(clientPlayerEntity.x), MathHelper.floor(clientPlayerEntity.y), MathHelper.floor(clientPlayerEntity.z));
        if (itemStack != null) {
            int n = Item.ITEMS[itemStack.itemId].getColorMultiplier(itemStack.getDamage());
            f3 = (float)(n >> 16 & 0xFF) / 255.0f;
            f2 = (float)(n >> 8 & 0xFF) / 255.0f;
            f = (float)(n & 0xFF) / 255.0f;
            GL11.glColor4f(f6 * f3, f6 * f2, f6 * f, 1.0f);
        } else {
            GL11.glColor4f(f6, f6, f6, 1.0f);
        }
        if (itemStack != null && itemStack.itemId == Item.MAP.id) {
            GL11.glPushMatrix();
            float f7 = 0.8f;
            f3 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f2 = MathHelper.sin(f3 * (float)Math.PI);
            f = MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI);
            GL11.glTranslatef(-f * 0.4f, MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI * 2.0f) * 0.2f, -f2 * 0.2f);
            f3 = 1.0f - f5 / 45.0f + 0.1f;
            if (f3 < 0.0f) {
                f3 = 0.0f;
            }
            if (f3 > 1.0f) {
                f3 = 1.0f;
            }
            f3 = -MathHelper.cos(f3 * (float)Math.PI) * 0.5f + 0.5f;
            GL11.glTranslatef(0.0f, 0.0f * f7 - (1.0f - f4) * 1.2f - f3 * 0.5f + 0.04f, -0.9f * f7);
            GL11.glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(f3 * -85.0f, 0.0f, 0.0f, 1.0f);
            GL11.glEnable(32826);
            GL11.glBindTexture(3553, this.minecraft.textureManager.downloadTexture(this.minecraft.player.skinUrl, this.minecraft.player.getTexture()));
            for (int i = 0; i < 2; ++i) {
                int n = i * 2 - 1;
                GL11.glPushMatrix();
                GL11.glTranslatef(-0.0f, -0.6f, 1.1f * (float)n);
                GL11.glRotatef(-45 * n, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
                GL11.glRotatef(59.0f, 0.0f, 0.0f, 1.0f);
                GL11.glRotatef(-65 * n, 0.0f, 1.0f, 0.0f);
                EntityRenderer entityRenderer = EntityRenderDispatcher.INSTANCE.get(this.minecraft.player);
                PlayerEntityRenderer playerEntityRenderer = (PlayerEntityRenderer)entityRenderer;
                float f8 = 1.0f;
                GL11.glScalef(f8, f8, f8);
                playerEntityRenderer.renderHand();
                GL11.glPopMatrix();
            }
            float f9 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f = MathHelper.sin(f9 * f9 * (float)Math.PI);
            float f10 = MathHelper.sin(MathHelper.sqrt(f9) * (float)Math.PI);
            GL11.glRotatef(-f * 20.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(-f10 * 20.0f, 0.0f, 0.0f, 1.0f);
            GL11.glRotatef(-f10 * 80.0f, 1.0f, 0.0f, 0.0f);
            f9 = 0.38f;
            GL11.glScalef(f9, f9, f9);
            GL11.glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
            GL11.glTranslatef(-1.0f, -1.0f, 0.0f);
            f = 0.015625f;
            GL11.glScalef(f, f, f);
            this.minecraft.textureManager.bindTexture(this.minecraft.textureManager.getTextureId("/misc/mapbg.png"));
            Tessellator tessellator = Tessellator.INSTANCE;
            GL11.glNormal3f(0.0f, 0.0f, -1.0f);
            tessellator.startQuads();
            int n = 7;
            tessellator.vertex(0 - n, 128 + n, 0.0, 0.0, 1.0);
            tessellator.vertex(128 + n, 128 + n, 0.0, 1.0, 1.0);
            tessellator.vertex(128 + n, 0 - n, 0.0, 1.0, 0.0);
            tessellator.vertex(0 - n, 0 - n, 0.0, 0.0, 0.0);
            tessellator.draw();
            MapState mapState = Item.MAP.getSavedMapState(itemStack, this.minecraft.world);
            this.mapRenderer.render(this.minecraft.player, this.minecraft.textureManager, mapState);
            GL11.glPopMatrix();
        } else if (itemStack != null) {
            GL11.glPushMatrix();
            float f11 = 0.8f;
            f3 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f2 = MathHelper.sin(f3 * (float)Math.PI);
            f = MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI);
            GL11.glTranslatef(-f * 0.4f, MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI * 2.0f) * 0.2f, -f2 * 0.2f);
            GL11.glTranslatef(0.7f * f11, -0.65f * f11 - (1.0f - f4) * 0.6f, -0.9f * f11);
            GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            GL11.glEnable(32826);
            f3 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f2 = MathHelper.sin(f3 * f3 * (float)Math.PI);
            f = MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI);
            GL11.glRotatef(-f2 * 20.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(-f * 20.0f, 0.0f, 0.0f, 1.0f);
            GL11.glRotatef(-f * 80.0f, 1.0f, 0.0f, 0.0f);
            f3 = 0.4f;
            GL11.glScalef(f3, f3, f3);
            if (itemStack.getItem().isHandheldRod()) {
                GL11.glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
            }
            this.renderItem(clientPlayerEntity, itemStack);
            GL11.glPopMatrix();
        } else {
            GL11.glPushMatrix();
            float f12 = 0.8f;
            f3 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f2 = MathHelper.sin(f3 * (float)Math.PI);
            f = MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI);
            GL11.glTranslatef(-f * 0.3f, MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI * 2.0f) * 0.4f, -f2 * 0.4f);
            GL11.glTranslatef(0.8f * f12, -0.75f * f12 - (1.0f - f4) * 0.6f, -0.9f * f12);
            GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            GL11.glEnable(32826);
            f3 = clientPlayerEntity.getHandSwingProgress(tickDelta);
            f2 = MathHelper.sin(f3 * f3 * (float)Math.PI);
            f = MathHelper.sin(MathHelper.sqrt(f3) * (float)Math.PI);
            GL11.glRotatef(f * 70.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(-f2 * 20.0f, 0.0f, 0.0f, 1.0f);
            GL11.glBindTexture(3553, this.minecraft.textureManager.downloadTexture(this.minecraft.player.skinUrl, this.minecraft.player.getTexture()));
            GL11.glTranslatef(-1.0f, 3.6f, 3.5f);
            GL11.glRotatef(120.0f, 0.0f, 0.0f, 1.0f);
            GL11.glRotatef(200.0f, 1.0f, 0.0f, 0.0f);
            GL11.glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
            GL11.glScalef(1.0f, 1.0f, 1.0f);
            GL11.glTranslatef(5.6f, 0.0f, 0.0f);
            EntityRenderer entityRenderer = EntityRenderDispatcher.INSTANCE.get(this.minecraft.player);
            PlayerEntityRenderer playerEntityRenderer = (PlayerEntityRenderer)entityRenderer;
            f = 1.0f;
            GL11.glScalef(f, f, f);
            playerEntityRenderer.renderHand();
            GL11.glPopMatrix();
        }
        GL11.glDisable(32826);
        Lighting.turnOff();
    }

    public void renderScreenOverlays(float tickDelta) {
        int n;
        GL11.glDisable(3008);
        if (this.minecraft.player.isOnFire()) {
            n = this.minecraft.textureManager.getTextureId("/terrain.png");
            GL11.glBindTexture(3553, n);
            this.renderFireOverlay(tickDelta);
        }
        if (this.minecraft.player.isInsideWall()) {
            n = MathHelper.floor(this.minecraft.player.x);
            int n2 = MathHelper.floor(this.minecraft.player.y);
            int n3 = MathHelper.floor(this.minecraft.player.z);
            int n4 = this.minecraft.textureManager.getTextureId("/terrain.png");
            GL11.glBindTexture(3553, n4);
            int n5 = this.minecraft.world.getBlockId(n, n2, n3);
            if (this.minecraft.world.shouldSuffocate(n, n2, n3)) {
                this.renderTexturedOverlay(tickDelta, Block.BLOCKS[n5].getTexture(2));
            } else {
                for (int i = 0; i < 8; ++i) {
                    int n6;
                    int n7;
                    float f = ((float)((i >> 0) % 2) - 0.5f) * this.minecraft.player.width * 0.9f;
                    float f2 = ((float)((i >> 1) % 2) - 0.5f) * this.minecraft.player.height * 0.2f;
                    float f3 = ((float)((i >> 2) % 2) - 0.5f) * this.minecraft.player.width * 0.9f;
                    int n8 = MathHelper.floor((float)n + f);
                    if (!this.minecraft.world.shouldSuffocate(n8, n7 = MathHelper.floor((float)n2 + f2), n6 = MathHelper.floor((float)n3 + f3))) continue;
                    n5 = this.minecraft.world.getBlockId(n8, n7, n6);
                }
            }
            if (Block.BLOCKS[n5] != null) {
                this.renderTexturedOverlay(tickDelta, Block.BLOCKS[n5].getTexture(2));
            }
        }
        if (this.minecraft.player.isInFluid(Material.WATER)) {
            n = this.minecraft.textureManager.getTextureId("/misc/water.png");
            GL11.glBindTexture(3553, n);
            this.renderUnderwaterOverlay(tickDelta);
        }
        GL11.glEnable(3008);
    }

    private void renderTexturedOverlay(float height, int textureId) {
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = this.minecraft.player.getBrightnessAtEyes(height);
        f = 0.1f;
        GL11.glColor4f(f, f, f, 0.5f);
        GL11.glPushMatrix();
        float f2 = -1.0f;
        float f3 = 1.0f;
        float f4 = -1.0f;
        float f5 = 1.0f;
        float f6 = -0.5f;
        float f7 = 0.0078125f;
        float f8 = (float)(textureId % 16) / 256.0f - f7;
        float f9 = ((float)(textureId % 16) + 15.99f) / 256.0f + f7;
        float f10 = (float)(textureId / 16) / 256.0f - f7;
        float f11 = ((float)(textureId / 16) + 15.99f) / 256.0f + f7;
        tessellator.startQuads();
        tessellator.vertex(f2, f4, f6, f9, f11);
        tessellator.vertex(f3, f4, f6, f8, f11);
        tessellator.vertex(f3, f5, f6, f8, f10);
        tessellator.vertex(f2, f5, f6, f9, f10);
        tessellator.draw();
        GL11.glPopMatrix();
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    private void renderUnderwaterOverlay(float delta) {
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = this.minecraft.player.getBrightnessAtEyes(delta);
        GL11.glColor4f(f, f, f, 0.5f);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        GL11.glPushMatrix();
        float f2 = 4.0f;
        float f3 = -1.0f;
        float f4 = 1.0f;
        float f5 = -1.0f;
        float f6 = 1.0f;
        float f7 = -0.5f;
        float f8 = -this.minecraft.player.yaw / 64.0f;
        float f9 = this.minecraft.player.pitch / 64.0f;
        tessellator.startQuads();
        tessellator.vertex(f3, f5, f7, f2 + f8, f2 + f9);
        tessellator.vertex(f4, f5, f7, 0.0f + f8, f2 + f9);
        tessellator.vertex(f4, f6, f7, 0.0f + f8, 0.0f + f9);
        tessellator.vertex(f3, f6, f7, f2 + f8, 0.0f + f9);
        tessellator.draw();
        GL11.glPopMatrix();
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
    }

    private void renderFireOverlay(float tickDelta) {
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        float f = 1.0f;
        for (int i = 0; i < 2; ++i) {
            GL11.glPushMatrix();
            int n = Block.FIRE.textureId + i * 16;
            int n2 = (n & 0xF) << 4;
            int n3 = n & 0xF0;
            float f2 = (float)n2 / 256.0f;
            float f3 = ((float)n2 + 15.99f) / 256.0f;
            float f4 = (float)n3 / 256.0f;
            float f5 = ((float)n3 + 15.99f) / 256.0f;
            float f6 = (0.0f - f) / 2.0f;
            float f7 = f6 + f;
            float f8 = 0.0f - f / 2.0f;
            float f9 = f8 + f;
            float f10 = -0.5f;
            GL11.glTranslatef((float)(-(i * 2 - 1)) * 0.24f, -0.3f, 0.0f);
            GL11.glRotatef((float)(i * 2 - 1) * 10.0f, 0.0f, 1.0f, 0.0f);
            tessellator.startQuads();
            tessellator.vertex(f6, f8, f10, f3, f5);
            tessellator.vertex(f7, f8, f10, f2, f5);
            tessellator.vertex(f7, f9, f10, f2, f4);
            tessellator.vertex(f6, f9, f10, f3, f4);
            tessellator.draw();
            GL11.glPopMatrix();
        }
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
    }

    public void tick() {
        float f;
        float f2;
        float f3;
        boolean bl;
        ItemStack itemStack;
        this.prevHeight = this.height;
        ClientPlayerEntity clientPlayerEntity = this.minecraft.player;
        ItemStack itemStack2 = itemStack = clientPlayerEntity.inventory.getSelectedItem();
        boolean bl2 = bl = this.slot == clientPlayerEntity.inventory.selectedSlot && itemStack2 == this.stack;
        if (this.stack == null && itemStack2 == null) {
            bl = true;
        }
        if (itemStack2 != null && this.stack != null && itemStack2 != this.stack && itemStack2.itemId == this.stack.itemId && itemStack2.getDamage() == this.stack.getDamage()) {
            this.stack = itemStack2;
            bl = true;
        }
        if ((f3 = (f2 = bl ? 1.0f : 0.0f) - this.height) < -(f = 0.4f)) {
            f3 = -f;
        }
        if (f3 > f) {
            f3 = f;
        }
        this.height += f3;
        if (this.height < 0.1f) {
            this.stack = itemStack2;
            this.slot = clientPlayerEntity.inventory.selectedSlot;
        }
    }

    public void place() {
        this.height = 0.0f;
    }

    public void use() {
        this.height = 0.0f;
    }
}

