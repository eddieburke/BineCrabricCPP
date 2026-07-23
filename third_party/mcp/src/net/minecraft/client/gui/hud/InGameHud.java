/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.hud;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.client.Minecraft;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.hud.ChatHudLine;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.client.util.ScreenScaler;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class InGameHud
extends DrawContext {
    private static ItemRenderer ITEM_RENDERER = new ItemRenderer();
    private List messages = new ArrayList();
    private Random random = new Random();
    private Minecraft minecraft;
    public String selectedName = null;
    private int ticks = 0;
    private String overlayMessage = "";
    private int overlayRemaining = 0;
    private boolean overlayTinted = false;
    public float progress;
    float vignetteDarkness = 1.0f;

    public InGameHud(Minecraft minecraft) {
        this.minecraft = minecraft;
    }

    public void render(float tickDelta, boolean screenOpen, int mouseX, int mouseY) {
        String string;
        int n;
        int n2;
        int n3;
        boolean bl;
        float f;
        ScreenScaler screenScaler = new ScreenScaler(this.minecraft.options, this.minecraft.displayWidth, this.minecraft.displayHeight);
        int n4 = screenScaler.getScaledWidth();
        int n5 = screenScaler.getScaledHeight();
        TextRenderer textRenderer = this.minecraft.textRenderer;
        this.minecraft.gameRenderer.setupHudRender();
        GL11.glEnable(3042);
        if (Minecraft.isFancyGraphicsEnabled()) {
            this.renderVignette(this.minecraft.player.getBrightnessAtEyes(tickDelta), n4, n5);
        }
        ItemStack itemStack = this.minecraft.player.inventory.getArmorStack(3);
        if (!this.minecraft.options.thirdPerson && itemStack != null && itemStack.itemId == Block.PUMPKIN.id) {
            this.renderPumpkinOverlay(n4, n5);
        }
        if ((f = this.minecraft.player.lastScreenDistortion + (this.minecraft.player.screenDistortion - this.minecraft.player.lastScreenDistortion) * tickDelta) > 0.0f) {
            this.renderPortalOverlay(f, n4, n5);
        }
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/gui.png"));
        PlayerInventory playerInventory = this.minecraft.player.inventory;
        this.zOffset = -90.0f;
        this.drawTexture(n4 / 2 - 91, n5 - 22, 0, 0, 182, 22);
        this.drawTexture(n4 / 2 - 91 - 1 + playerInventory.selectedSlot * 20, n5 - 22 - 1, 0, 22, 24, 22);
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/icons.png"));
        GL11.glEnable(3042);
        GL11.glBlendFunc(775, 769);
        this.drawTexture(n4 / 2 - 7, n5 / 2 - 7, 0, 0, 16, 16);
        GL11.glDisable(3042);
        boolean bl2 = bl = this.minecraft.player.hearts / 3 % 2 == 1;
        if (this.minecraft.player.hearts < 10) {
            bl = false;
        }
        int n6 = this.minecraft.player.health;
        int n7 = this.minecraft.player.lastHealth;
        this.random.setSeed(this.ticks * 312871);
        if (this.minecraft.interactionManager.canBeRendered()) {
            int n8;
            n3 = this.minecraft.player.getTotalArmorDurability();
            for (n2 = 0; n2 < 10; ++n2) {
                n = n5 - 32;
                if (n3 > 0) {
                    n8 = n4 / 2 + 91 - n2 * 8 - 9;
                    if (n2 * 2 + 1 < n3) {
                        this.drawTexture(n8, n, 34, 9, 9, 9);
                    }
                    if (n2 * 2 + 1 == n3) {
                        this.drawTexture(n8, n, 25, 9, 9, 9);
                    }
                    if (n2 * 2 + 1 > n3) {
                        this.drawTexture(n8, n, 16, 9, 9, 9);
                    }
                }
                n8 = 0;
                if (bl) {
                    n8 = 1;
                }
                int n9 = n4 / 2 - 91 + n2 * 8;
                if (n6 <= 4) {
                    n += this.random.nextInt(2);
                }
                this.drawTexture(n9, n, 16 + n8 * 9, 0, 9, 9);
                if (bl) {
                    if (n2 * 2 + 1 < n7) {
                        this.drawTexture(n9, n, 70, 0, 9, 9);
                    }
                    if (n2 * 2 + 1 == n7) {
                        this.drawTexture(n9, n, 79, 0, 9, 9);
                    }
                }
                if (n2 * 2 + 1 < n6) {
                    this.drawTexture(n9, n, 52, 0, 9, 9);
                }
                if (n2 * 2 + 1 != n6) continue;
                this.drawTexture(n9, n, 61, 0, 9, 9);
            }
            if (this.minecraft.player.isInFluid(Material.WATER)) {
                n2 = (int)Math.ceil((double)(this.minecraft.player.air - 2) * 10.0 / 300.0);
                n = (int)Math.ceil((double)this.minecraft.player.air * 10.0 / 300.0) - n2;
                for (n8 = 0; n8 < n2 + n; ++n8) {
                    if (n8 < n2) {
                        this.drawTexture(n4 / 2 - 91 + n8 * 8, n5 - 32 - 9, 16, 18, 9, 9);
                        continue;
                    }
                    this.drawTexture(n4 / 2 - 91 + n8 * 8, n5 - 32 - 9, 25, 18, 9, 9);
                }
            }
        }
        GL11.glDisable(3042);
        GL11.glEnable(32826);
        GL11.glPushMatrix();
        GL11.glRotatef(120.0f, 1.0f, 0.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        for (n3 = 0; n3 < 9; ++n3) {
            n2 = n4 / 2 - 90 + n3 * 20 + 2;
            n = n5 - 16 - 3;
            this.renderHotbarItem(n3, n2, n, tickDelta);
        }
        Lighting.turnOff();
        GL11.glDisable(32826);
        if (this.minecraft.player.getSleepTimer() > 0) {
            GL11.glDisable(2929);
            GL11.glDisable(3008);
            n3 = this.minecraft.player.getSleepTimer();
            float f2 = (float)n3 / 100.0f;
            if (f2 > 1.0f) {
                f2 = 1.0f - (float)(n3 - 100) / 10.0f;
            }
            n = (int)(220.0f * f2) << 24 | 0x101020;
            this.fill(0, 0, n4, n5, n);
            GL11.glEnable(3008);
            GL11.glEnable(2929);
        }
        if (this.minecraft.options.debugHud) {
            GL11.glPushMatrix();
            if (Minecraft.failedSessionCheckTime > 0L) {
                GL11.glTranslatef(0.0f, 32.0f, 0.0f);
            }
            textRenderer.drawWithShadow("Minecraft Beta 1.7.3 (" + this.minecraft.debugText + ")", 2, 2, 0xFFFFFF);
            textRenderer.drawWithShadow(this.minecraft.getRenderChunkDebugInfo(), 2, 12, 0xFFFFFF);
            textRenderer.drawWithShadow(this.minecraft.getRenderEntityDebugInfo(), 2, 22, 0xFFFFFF);
            textRenderer.drawWithShadow(this.minecraft.getWorldDebugInfo(), 2, 32, 0xFFFFFF);
            textRenderer.drawWithShadow(this.minecraft.getChunkSourceDebugInfo(), 2, 42, 0xFFFFFF);
            long l = Runtime.getRuntime().maxMemory();
            long l2 = Runtime.getRuntime().totalMemory();
            long l3 = Runtime.getRuntime().freeMemory();
            long l4 = l2 - l3;
            string = "Used memory: " + l4 * 100L / l + "% (" + l4 / 1024L / 1024L + "MB) of " + l / 1024L / 1024L + "MB";
            this.drawTextWithShadow(textRenderer, string, n4 - textRenderer.getWidth(string) - 2, 2, 0xE0E0E0);
            string = "Allocated memory: " + l2 * 100L / l + "% (" + l2 / 1024L / 1024L + "MB)";
            this.drawTextWithShadow(textRenderer, string, n4 - textRenderer.getWidth(string) - 2, 12, 0xE0E0E0);
            this.drawTextWithShadow(textRenderer, "x: " + this.minecraft.player.x, 2, 64, 0xE0E0E0);
            this.drawTextWithShadow(textRenderer, "y: " + this.minecraft.player.y, 2, 72, 0xE0E0E0);
            this.drawTextWithShadow(textRenderer, "z: " + this.minecraft.player.z, 2, 80, 0xE0E0E0);
            this.drawTextWithShadow(textRenderer, "f: " + (MathHelper.floor((double)(this.minecraft.player.yaw * 4.0f / 360.0f) + 0.5) & 3), 2, 88, 0xE0E0E0);
            GL11.glPopMatrix();
        }
        if (this.overlayRemaining > 0) {
            float f3 = (float)this.overlayRemaining - tickDelta;
            int n10 = (int)(f3 * 256.0f / 20.0f);
            if (n10 > 255) {
                n10 = 255;
            }
            if (n10 > 0) {
                GL11.glPushMatrix();
                GL11.glTranslatef(n4 / 2, n5 - 48, 0.0f);
                GL11.glEnable(3042);
                GL11.glBlendFunc(770, 771);
                int n11 = 0xFFFFFF;
                if (this.overlayTinted) {
                    n11 = Color.HSBtoRGB(f3 / 50.0f, 0.7f, 0.6f) & 0xFFFFFF;
                }
                textRenderer.draw(this.overlayMessage, -textRenderer.getWidth(this.overlayMessage) / 2, -4, n11 + (n10 << 24));
                GL11.glDisable(3042);
                GL11.glPopMatrix();
            }
        }
        int n12 = 10;
        boolean bl3 = false;
        if (this.minecraft.currentScreen instanceof ChatScreen) {
            n12 = 20;
            bl3 = true;
        }
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        GL11.glDisable(3008);
        GL11.glPushMatrix();
        GL11.glTranslatef(0.0f, n5 - 48, 0.0f);
        for (int i = 0; i < this.messages.size() && i < n12; ++i) {
            if (((ChatHudLine)this.messages.get((int)i)).age >= 200 && !bl3) continue;
            double d = (double)((ChatHudLine)this.messages.get((int)i)).age / 200.0;
            d = 1.0 - d;
            if ((d *= 10.0) < 0.0) {
                d = 0.0;
            }
            if (d > 1.0) {
                d = 1.0;
            }
            d *= d;
            int n13 = (int)(255.0 * d);
            if (bl3) {
                n13 = 255;
            }
            if (n13 <= 0) continue;
            int n14 = 2;
            int n15 = -i * 9;
            string = ((ChatHudLine)this.messages.get((int)i)).text;
            this.fill(n14, n15 - 1, n14 + 320, n15 + 8, n13 / 2 << 24);
            GL11.glEnable(3042);
            textRenderer.drawWithShadow(string, n14, n15, 0xFFFFFF + (n13 << 24));
        }
        GL11.glPopMatrix();
        GL11.glEnable(3008);
        GL11.glDisable(3042);
    }

    private void renderPumpkinOverlay(int i, int j) {
        GL11.glDisable(2929);
        GL11.glDepthMask(false);
        GL11.glBlendFunc(770, 771);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3008);
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("%blur%/misc/pumpkinblur.png"));
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(0.0, j, -90.0, 0.0, 1.0);
        tessellator.vertex(i, j, -90.0, 1.0, 1.0);
        tessellator.vertex(i, 0.0, -90.0, 1.0, 0.0);
        tessellator.vertex(0.0, 0.0, -90.0, 0.0, 0.0);
        tessellator.draw();
        GL11.glDepthMask(true);
        GL11.glEnable(2929);
        GL11.glEnable(3008);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    private void renderVignette(float f, int i, int j) {
        if ((f = 1.0f - f) < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        this.vignetteDarkness = (float)((double)this.vignetteDarkness + (double)(f - this.vignetteDarkness) * 0.01);
        GL11.glDisable(2929);
        GL11.glDepthMask(false);
        GL11.glBlendFunc(0, 769);
        GL11.glColor4f(this.vignetteDarkness, this.vignetteDarkness, this.vignetteDarkness, 1.0f);
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("%blur%/misc/vignette.png"));
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(0.0, j, -90.0, 0.0, 1.0);
        tessellator.vertex(i, j, -90.0, 1.0, 1.0);
        tessellator.vertex(i, 0.0, -90.0, 1.0, 0.0);
        tessellator.vertex(0.0, 0.0, -90.0, 0.0, 0.0);
        tessellator.draw();
        GL11.glDepthMask(true);
        GL11.glEnable(2929);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glBlendFunc(770, 771);
    }

    private void renderPortalOverlay(float f, int i, int j) {
        if (f < 1.0f) {
            f *= f;
            f *= f;
            f = f * 0.8f + 0.2f;
        }
        GL11.glDisable(3008);
        GL11.glDisable(2929);
        GL11.glDepthMask(false);
        GL11.glBlendFunc(770, 771);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, f);
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/terrain.png"));
        float f2 = (float)(Block.NETHER_PORTAL.textureId % 16) / 16.0f;
        float f3 = (float)(Block.NETHER_PORTAL.textureId / 16) / 16.0f;
        float f4 = (float)(Block.NETHER_PORTAL.textureId % 16 + 1) / 16.0f;
        float f5 = (float)(Block.NETHER_PORTAL.textureId / 16 + 1) / 16.0f;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(0.0, j, -90.0, f2, f5);
        tessellator.vertex(i, j, -90.0, f4, f5);
        tessellator.vertex(i, 0.0, -90.0, f4, f3);
        tessellator.vertex(0.0, 0.0, -90.0, f2, f3);
        tessellator.draw();
        GL11.glDepthMask(true);
        GL11.glEnable(2929);
        GL11.glEnable(3008);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    private void renderHotbarItem(int slot, int x, int y, float f) {
        ItemStack itemStack = this.minecraft.player.inventory.main[slot];
        if (itemStack == null) {
            return;
        }
        float f2 = (float)itemStack.bobbingAnimationTime - f;
        if (f2 > 0.0f) {
            GL11.glPushMatrix();
            float f3 = 1.0f + f2 / 5.0f;
            GL11.glTranslatef(x + 8, y + 12, 0.0f);
            GL11.glScalef(1.0f / f3, (f3 + 1.0f) / 2.0f, 1.0f);
            GL11.glTranslatef(-(x + 8), -(y + 12), 0.0f);
        }
        ITEM_RENDERER.renderGuiItem(this.minecraft.textRenderer, this.minecraft.textureManager, itemStack, x, y);
        if (f2 > 0.0f) {
            GL11.glPopMatrix();
        }
        ITEM_RENDERER.renderGuiItemDecoration(this.minecraft.textRenderer, this.minecraft.textureManager, itemStack, x, y);
    }

    public void tick() {
        if (this.overlayRemaining > 0) {
            --this.overlayRemaining;
        }
        ++this.ticks;
        for (int i = 0; i < this.messages.size(); ++i) {
            ++((ChatHudLine)this.messages.get((int)i)).age;
        }
    }

    public void clearChat() {
        this.messages.clear();
    }

    public void addChatMessage(String message) {
        while (this.minecraft.textRenderer.getWidth(message) > 320) {
            int n;
            for (n = 1; n < message.length() && this.minecraft.textRenderer.getWidth(message.substring(0, n + 1)) <= 320; ++n) {
            }
            this.addChatMessage(message.substring(0, n));
            message = message.substring(n);
        }
        this.messages.add(0, new ChatHudLine(message));
        while (this.messages.size() > 50) {
            this.messages.remove(this.messages.size() - 1);
        }
    }

    public void setRecordPlayingOverlay(String record) {
        this.overlayMessage = "Now playing: " + record;
        this.overlayRemaining = 60;
        this.overlayTinted = true;
    }

    public void addTranslatedChatMessage(String text) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        String string = translationStorage.get(text);
        this.addChatMessage(string);
    }
}

