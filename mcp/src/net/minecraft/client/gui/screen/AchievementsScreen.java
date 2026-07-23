/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievement;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.OptionButtonWidget;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.stat.PlayerStats;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class AchievementsScreen
extends Screen {
    private static final int MIN_COLUMN = Achievements.minColumn * 24 - 112;
    private static final int MIN_ROW = Achievements.minRow * 24 - 112;
    private static final int MAX_COLUMN = Achievements.maxColumn * 24 - 77;
    private static final int MAX_ROW = Achievements.maxRow * 24 - 77;
    protected int iconWidth = 256;
    protected int iconHeight = 202;
    protected int prevMouseX = 0;
    protected int prevMouseY = 0;
    protected double mouseX;
    protected double mouseY;
    protected double scaledMouseDx;
    protected double scaledMouseDy;
    protected double scrollX;
    protected double scrollY;
    private int scroll = 0;
    private PlayerStats stats;

    public AchievementsScreen(PlayerStats stats) {
        this.stats = stats;
        int n = 141;
        int n2 = 141;
        this.scaledMouseDx = this.scrollX = (double)(Achievements.OPEN_INVENTORY.column * 24 - n / 2 - 12);
        this.mouseX = this.scrollX;
        this.scaledMouseDy = this.scrollY = (double)(Achievements.OPEN_INVENTORY.row * 24 - n2 / 2);
        this.mouseY = this.scrollY;
    }

    public void init() {
        this.buttons.clear();
        this.buttons.add(new OptionButtonWidget(1, this.width / 2 + 24, this.height / 2 + 74, 80, 20, I18n.getTranslation("gui.done")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 1) {
            this.minecraft.setScreen(null);
            this.minecraft.lockMouse();
        }
        super.buttonClicked(button);
    }

    protected void keyPressed(char character, int keyCode) {
        if (keyCode == this.minecraft.options.inventoryKey.code) {
            this.minecraft.setScreen(null);
            this.minecraft.lockMouse();
        } else {
            super.keyPressed(character, keyCode);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        if (Mouse.isButtonDown(0)) {
            int n = (this.width - this.iconWidth) / 2;
            int n2 = (this.height - this.iconHeight) / 2;
            int n3 = n + 8;
            int n4 = n2 + 17;
            if ((this.scroll == 0 || this.scroll == 1) && mouseX >= n3 && mouseX < n3 + 224 && mouseY >= n4 && mouseY < n4 + 155) {
                if (this.scroll == 0) {
                    this.scroll = 1;
                } else {
                    this.scaledMouseDx -= (double)(mouseX - this.prevMouseX);
                    this.scaledMouseDy -= (double)(mouseY - this.prevMouseY);
                    this.scrollX = this.mouseX = this.scaledMouseDx;
                    this.scrollY = this.mouseY = this.scaledMouseDy;
                }
                this.prevMouseX = mouseX;
                this.prevMouseY = mouseY;
            }
            if (this.scrollX < (double)MIN_COLUMN) {
                this.scrollX = MIN_COLUMN;
            }
            if (this.scrollY < (double)MIN_ROW) {
                this.scrollY = MIN_ROW;
            }
            if (this.scrollX >= (double)MAX_COLUMN) {
                this.scrollX = MAX_COLUMN - 1;
            }
            if (this.scrollY >= (double)MAX_ROW) {
                this.scrollY = MAX_ROW - 1;
            }
        } else {
            this.scroll = 0;
        }
        this.renderBackground();
        this.renderIcons(mouseX, mouseY, delta);
        GL11.glDisable(2896);
        GL11.glDisable(2929);
        this.setTitle();
        GL11.glEnable(2896);
        GL11.glEnable(2929);
    }

    public void tick() {
        this.mouseX = this.scaledMouseDx;
        this.mouseY = this.scaledMouseDy;
        double d = this.scrollX - this.scaledMouseDx;
        double d2 = this.scrollY - this.scaledMouseDy;
        if (d * d + d2 * d2 < 4.0) {
            this.scaledMouseDx += d;
            this.scaledMouseDy += d2;
        } else {
            this.scaledMouseDx += d * 0.85;
            this.scaledMouseDy += d2 * 0.85;
        }
    }

    protected void setTitle() {
        int n = (this.width - this.iconWidth) / 2;
        int n2 = (this.height - this.iconHeight) / 2;
        this.textRenderer.draw("Achievements", n + 15, n2 + 5, 0x404040);
    }

    protected void renderIcons(int mouseX, int mouseY, float tickDelta) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5 = MathHelper.floor(this.mouseX + (this.scaledMouseDx - this.mouseX) * (double)tickDelta);
        int n6 = MathHelper.floor(this.mouseY + (this.scaledMouseDy - this.mouseY) * (double)tickDelta);
        if (n5 < MIN_COLUMN) {
            n5 = MIN_COLUMN;
        }
        if (n6 < MIN_ROW) {
            n6 = MIN_ROW;
        }
        if (n5 >= MAX_COLUMN) {
            n5 = MAX_COLUMN - 1;
        }
        if (n6 >= MAX_ROW) {
            n6 = MAX_ROW - 1;
        }
        int n7 = this.minecraft.textureManager.getTextureId("/terrain.png");
        int n8 = this.minecraft.textureManager.getTextureId("/achievement/bg.png");
        int n9 = (this.width - this.iconWidth) / 2;
        int n10 = (this.height - this.iconHeight) / 2;
        int n11 = n9 + 16;
        int n12 = n10 + 17;
        this.zOffset = 0.0f;
        GL11.glDepthFunc(518);
        GL11.glPushMatrix();
        GL11.glTranslatef(0.0f, 0.0f, -200.0f);
        GL11.glEnable(3553);
        GL11.glDisable(2896);
        GL11.glEnable(32826);
        GL11.glEnable(2903);
        this.minecraft.textureManager.bindTexture(n7);
        int n13 = n5 + 288 >> 4;
        int n14 = n6 + 288 >> 4;
        int n15 = (n5 + 288) % 16;
        int n16 = (n6 + 288) % 16;
        Random random = new Random();
        int n17 = 0;
        while (n17 * 16 - n16 < 155) {
            float f = Math.max(0.1f, 0.6f - (float)(n14 + n17) / 25.0f * 0.3f);
            GL11.glColor4f(f, f, f, 1.0f);
            int n18 = 0;
            while (n18 * 16 - n15 < 224) {
                random.setSeed(1234 + n13 + n18);
                random.nextInt();
                int n19 = random.nextInt(1 + n14 + n17) + (n14 + n17) / 2;
                int n20 = Block.SAND.textureId;
                if (n19 > 37 || n14 + n17 == 35) {
                    n20 = Block.BEDROCK.textureId;
                } else if (n19 == 22) {
                    n20 = random.nextInt(2) == 0 ? Block.DIAMOND_ORE.textureId : Block.REDSTONE_ORE.textureId;
                } else if (n19 == 10) {
                    n20 = Block.IRON_ORE.textureId;
                } else if (n19 == 8) {
                    n20 = Block.COAL_ORE.textureId;
                } else if (n19 > 4) {
                    n20 = Block.STONE.textureId;
                } else if (n19 > 0) {
                    n20 = Block.DIRT.textureId;
                }
                this.drawTexture(n11 + n18 * 16 - n15, n12 + n17 * 16 - n16, n20 % 16 << 4, n20 >> 4 << 4, 16, 16);
                ++n18;
            }
            ++n17;
        }
        GL11.glEnable(2929);
        GL11.glDepthFunc(515);
        GL11.glDisable(3553);
        for (n13 = 0; n13 < Achievements.ACHIEVEMENTS.size(); ++n13) {
            int n21;
            Achievement achievement = (Achievement)Achievements.ACHIEVEMENTS.get(n13);
            if (achievement.parent == null) continue;
            n15 = achievement.column * 24 - n5 + 11 + n11;
            n16 = achievement.row * 24 - n6 + 11 + n12;
            n4 = achievement.parent.column * 24 - n5 + 11 + n11;
            n3 = achievement.parent.row * 24 - n6 + 11 + n12;
            n2 = 0;
            n = this.stats.hasAchievement(achievement);
            boolean bl = this.stats.hasParentAchievement(achievement);
            int n22 = n21 = Math.sin((double)(System.currentTimeMillis() % 600L) / 600.0 * Math.PI * 2.0) > 0.6 ? 255 : 130;
            n2 = n != 0 ? -9408400 : (bl ? 65280 + (n21 << 24) : -16777216);
            this.drawHorizontalLine(n15, n4, n16, n2);
            this.drawVerticalLine(n4, n16, n3, n2);
        }
        Achievement achievement = null;
        ItemRenderer itemRenderer = new ItemRenderer();
        GL11.glPushMatrix();
        GL11.glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        GL11.glDisable(2896);
        GL11.glEnable(32826);
        GL11.glEnable(2903);
        for (n15 = 0; n15 < Achievements.ACHIEVEMENTS.size(); ++n15) {
            float f;
            Achievement achievement2 = (Achievement)Achievements.ACHIEVEMENTS.get(n15);
            n4 = achievement2.column * 24 - n5;
            n3 = achievement2.row * 24 - n6;
            if (n4 < -24 || n3 < -24 || n4 > 224 || n3 > 155) continue;
            if (this.stats.hasAchievement(achievement2)) {
                f = 1.0f;
                GL11.glColor4f(f, f, f, 1.0f);
            } else if (this.stats.hasParentAchievement(achievement2)) {
                f = Math.sin((double)(System.currentTimeMillis() % 600L) / 600.0 * Math.PI * 2.0) < 0.6 ? 0.6f : 0.8f;
                GL11.glColor4f(f, f, f, 1.0f);
            } else {
                f = 0.3f;
                GL11.glColor4f(f, f, f, 1.0f);
            }
            this.minecraft.textureManager.bindTexture(n8);
            n2 = n11 + n4;
            n = n12 + n3;
            if (achievement2.isChallenge()) {
                this.drawTexture(n2 - 2, n - 2, 26, 202, 26, 26);
            } else {
                this.drawTexture(n2 - 2, n - 2, 0, 202, 26, 26);
            }
            if (!this.stats.hasParentAchievement(achievement2)) {
                float f2 = 0.1f;
                GL11.glColor4f(f2, f2, f2, 1.0f);
                itemRenderer.useCustomDisplayColor = false;
            }
            GL11.glEnable(2896);
            GL11.glEnable(2884);
            itemRenderer.renderGuiItem(this.minecraft.textRenderer, this.minecraft.textureManager, achievement2.icon, n2 + 3, n + 3);
            GL11.glDisable(2896);
            if (!this.stats.hasParentAchievement(achievement2)) {
                itemRenderer.useCustomDisplayColor = true;
            }
            GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            if (mouseX < n11 || mouseY < n12 || mouseX >= n11 + 224 || mouseY >= n12 + 155 || mouseX < n2 || mouseX > n2 + 22 || mouseY < n || mouseY > n + 22) continue;
            achievement = achievement2;
        }
        GL11.glDisable(2929);
        GL11.glEnable(3042);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n8);
        this.drawTexture(n9, n10, 0, 0, this.iconWidth, this.iconHeight);
        GL11.glPopMatrix();
        this.zOffset = 0.0f;
        GL11.glDepthFunc(515);
        GL11.glDisable(2929);
        GL11.glEnable(3553);
        super.render(mouseX, mouseY, tickDelta);
        if (achievement != null) {
            Achievement achievement3 = achievement;
            String string = achievement3.stringId;
            String string2 = achievement3.getTranslatedDescription();
            n3 = mouseX + 12;
            n2 = mouseY - 4;
            if (this.stats.hasParentAchievement(achievement3)) {
                n = Math.max(this.textRenderer.getWidth(string), 120);
                int n23 = this.textRenderer.splitAndGetHeight(string2, n);
                if (this.stats.hasAchievement(achievement3)) {
                    n23 += 12;
                }
                this.fillGradient(n3 - 3, n2 - 3, n3 + n + 3, n2 + n23 + 3 + 12, -1073741824, -1073741824);
                this.textRenderer.drawSplit(string2, n3, n2 + 12, n, -6250336);
                if (this.stats.hasAchievement(achievement3)) {
                    this.textRenderer.drawWithShadow(I18n.getTranslation("achievement.taken"), n3, n2 + n23 + 4, -7302913);
                }
            } else {
                n = Math.max(this.textRenderer.getWidth(string), 120);
                String string3 = I18n.getTranslation("achievement.requires", achievement3.parent.stringId);
                int n24 = this.textRenderer.splitAndGetHeight(string3, n);
                this.fillGradient(n3 - 3, n2 - 3, n3 + n + 3, n2 + n24 + 12 + 3, -1073741824, -1073741824);
                this.textRenderer.drawSplit(string3, n3, n2 + 12, n, -9416624);
            }
            this.textRenderer.drawWithShadow(string, n3, n2, this.stats.hasParentAchievement(achievement3) ? (achievement3.isChallenge() ? -128 : -1) : (achievement3.isChallenge() ? -8355776 : -8355712));
        }
        GL11.glEnable(2929);
        GL11.glEnable(2896);
        Lighting.turnOff();
    }

    public boolean shouldPause() {
        return true;
    }
}

