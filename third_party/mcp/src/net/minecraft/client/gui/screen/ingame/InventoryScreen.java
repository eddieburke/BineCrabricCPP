/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.ingame;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievements;
import net.minecraft.client.gui.screen.AchievementsScreen;
import net.minecraft.client.gui.screen.StatsScreen;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.entity.player.PlayerEntity;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class InventoryScreen
extends HandledScreen {
    private float mouseX;
    private float mouseY;

    public InventoryScreen(PlayerEntity player) {
        super(player.playerScreenHandler);
        this.passEvents = true;
        player.increaseStat(Achievements.OPEN_INVENTORY, 1);
    }

    public void init() {
        this.buttons.clear();
    }

    protected void drawForeground() {
        this.textRenderer.draw("Crafting", 86, 16, 0x404040);
    }

    public void render(int mouseX, int mouseY, float delta) {
        super.render(mouseX, mouseY, delta);
        this.mouseX = mouseX;
        this.mouseY = mouseY;
    }

    protected void drawBackground(float tickDelta) {
        int n = this.minecraft.textureManager.getTextureId("/gui/inventory.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n);
        int n2 = (this.width - this.backgroundWidth) / 2;
        int n3 = (this.height - this.backgroundHeight) / 2;
        this.drawTexture(n2, n3, 0, 0, this.backgroundWidth, this.backgroundHeight);
        GL11.glEnable(32826);
        GL11.glEnable(2903);
        GL11.glPushMatrix();
        GL11.glTranslatef(n2 + 51, n3 + 75, 50.0f);
        float f = 30.0f;
        GL11.glScalef(-f, f, f);
        GL11.glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        float f2 = this.minecraft.player.bodyYaw;
        float f3 = this.minecraft.player.yaw;
        float f4 = this.minecraft.player.pitch;
        float f5 = (float)(n2 + 51) - this.mouseX;
        float f6 = (float)(n3 + 75 - 50) - this.mouseY;
        GL11.glRotatef(135.0f, 0.0f, 1.0f, 0.0f);
        Lighting.turnOn();
        GL11.glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-((float)Math.atan(f6 / 40.0f)) * 20.0f, 1.0f, 0.0f, 0.0f);
        this.minecraft.player.bodyYaw = (float)Math.atan(f5 / 40.0f) * 20.0f;
        this.minecraft.player.yaw = (float)Math.atan(f5 / 40.0f) * 40.0f;
        this.minecraft.player.pitch = -((float)Math.atan(f6 / 40.0f)) * 20.0f;
        this.minecraft.player.minBrightness = 1.0f;
        GL11.glTranslatef(0.0f, this.minecraft.player.standingEyeHeight, 0.0f);
        EntityRenderDispatcher.INSTANCE.yaw = 180.0f;
        EntityRenderDispatcher.INSTANCE.render(this.minecraft.player, 0.0, 0.0, 0.0, 0.0f, 1.0f);
        this.minecraft.player.minBrightness = 0.0f;
        this.minecraft.player.bodyYaw = f2;
        this.minecraft.player.yaw = f3;
        this.minecraft.player.pitch = f4;
        GL11.glPopMatrix();
        Lighting.turnOff();
        GL11.glDisable(32826);
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            this.minecraft.setScreen(new AchievementsScreen(this.minecraft.stats));
        }
        if (button.id == 1) {
            this.minecraft.setScreen(new StatsScreen(this, this.minecraft.stats));
        }
    }
}

