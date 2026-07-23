/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.hud.toast;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievement;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.client.util.ScreenScaler;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class AchievementToast
extends DrawContext {
    private Minecraft client;
    private int width;
    private int height;
    private String title;
    private String description;
    private Achievement achievement;
    private long startTime;
    private ItemRenderer itemRenderer;
    private boolean tutorial;

    public AchievementToast(Minecraft client) {
        this.client = client;
        this.itemRenderer = new ItemRenderer();
    }

    public void set(Achievement achievement) {
        this.title = I18n.getTranslation("achievement.get");
        this.description = achievement.stringId;
        this.startTime = System.currentTimeMillis();
        this.achievement = achievement;
        this.tutorial = false;
    }

    public void setTutorial(Achievement achievement) {
        this.title = achievement.stringId;
        this.description = achievement.getTranslatedDescription();
        this.startTime = System.currentTimeMillis() - 2500L;
        this.achievement = achievement;
        this.tutorial = true;
    }

    private void render() {
        GL11.glViewport(0, 0, this.client.displayWidth, this.client.displayHeight);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        this.width = this.client.displayWidth;
        this.height = this.client.displayHeight;
        ScreenScaler screenScaler = new ScreenScaler(this.client.options, this.client.displayWidth, this.client.displayHeight);
        this.width = screenScaler.getScaledWidth();
        this.height = screenScaler.getScaledHeight();
        GL11.glClear(256);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glOrtho(0.0, this.width, this.height, 0.0, 1000.0, 3000.0);
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        GL11.glTranslatef(0.0f, 0.0f, -2000.0f);
    }

    public void tick() {
        if (Minecraft.failedSessionCheckTime > 0L) {
            GL11.glDisable(2929);
            GL11.glDepthMask(false);
            Lighting.turnOff();
            this.render();
            String string = "Minecraft Beta 1.7.3   Unlicensed Copy :(";
            String string2 = "(Or logged in from another location)";
            String string3 = "Purchase at minecraft.net";
            this.client.textRenderer.drawWithShadow(string, 2, 2, 0xFFFFFF);
            this.client.textRenderer.drawWithShadow(string2, 2, 11, 0xFFFFFF);
            this.client.textRenderer.drawWithShadow(string3, 2, 20, 0xFFFFFF);
            GL11.glDepthMask(true);
            GL11.glEnable(2929);
        }
        if (this.achievement == null || this.startTime == 0L) {
            return;
        }
        double d = (double)(System.currentTimeMillis() - this.startTime) / 3000.0;
        if (!this.tutorial && !this.tutorial && (d < 0.0 || d > 1.0)) {
            this.startTime = 0L;
            return;
        }
        this.render();
        GL11.glDisable(2929);
        GL11.glDepthMask(false);
        double d2 = d * 2.0;
        if (d2 > 1.0) {
            d2 = 2.0 - d2;
        }
        d2 *= 4.0;
        if ((d2 = 1.0 - d2) < 0.0) {
            d2 = 0.0;
        }
        d2 *= d2;
        d2 *= d2;
        int n = this.width - 160;
        int n2 = 0 - (int)(d2 * 36.0);
        int n3 = this.client.textureManager.getTextureId("/achievement/bg.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glEnable(3553);
        GL11.glBindTexture(3553, n3);
        GL11.glDisable(2896);
        this.drawTexture(n, n2, 96, 202, 160, 32);
        if (this.tutorial) {
            this.client.textRenderer.drawSplit(this.description, n + 30, n2 + 7, 120, -1);
        } else {
            this.client.textRenderer.draw(this.title, n + 30, n2 + 7, -256);
            this.client.textRenderer.draw(this.description, n + 30, n2 + 18, -1);
        }
        GL11.glPushMatrix();
        GL11.glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        GL11.glDisable(2896);
        GL11.glEnable(32826);
        GL11.glEnable(2903);
        GL11.glEnable(2896);
        this.itemRenderer.renderGuiItem(this.client.textRenderer, this.client.textureManager, this.achievement.icon, n + 8, n2 + 8);
        GL11.glDisable(2896);
        GL11.glDepthMask(true);
        GL11.glEnable(2929);
    }
}

