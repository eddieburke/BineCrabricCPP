/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.AchievementsScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.StatsScreen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.screen.option.OptionsScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.stat.Stats;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class GameMenuScreen
extends Screen {
    private int saveStep = 0;
    private int ticks = 0;

    public void init() {
        this.saveStep = 0;
        this.buttons.clear();
        int n = -16;
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height / 4 + 120 + n, "Save and quit to title"));
        if (this.minecraft.isWorldRemote()) {
            ((ButtonWidget)this.buttons.get((int)0)).text = "Disconnect";
        }
        this.buttons.add(new ButtonWidget(4, this.width / 2 - 100, this.height / 4 + 24 + n, "Back to game"));
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 96 + n, "Options..."));
        this.buttons.add(new ButtonWidget(5, this.width / 2 - 100, this.height / 4 + 48 + n, 98, 20, I18n.getTranslation("gui.achievements")));
        this.buttons.add(new ButtonWidget(6, this.width / 2 + 2, this.height / 4 + 48 + n, 98, 20, I18n.getTranslation("gui.stats")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            this.minecraft.setScreen(new OptionsScreen(this, this.minecraft.options));
        }
        if (button.id == 1) {
            this.minecraft.stats.increment(Stats.LEAVE_GAME, 1);
            if (this.minecraft.isWorldRemote()) {
                this.minecraft.world.disconnect();
            }
            this.minecraft.setWorld(null);
            this.minecraft.setScreen(new TitleScreen());
        }
        if (button.id == 4) {
            this.minecraft.setScreen(null);
            this.minecraft.lockMouse();
        }
        if (button.id == 5) {
            this.minecraft.setScreen(new AchievementsScreen(this.minecraft.stats));
        }
        if (button.id == 6) {
            this.minecraft.setScreen(new StatsScreen(this, this.minecraft.stats));
        }
    }

    public void tick() {
        super.tick();
        ++this.ticks;
    }

    public void render(int mouseX, int mouseY, float delta) {
        boolean bl;
        this.renderBackground();
        boolean bl2 = bl = !this.minecraft.world.attemptSaving(this.saveStep++);
        if (bl || this.ticks < 20) {
            float f = ((float)(this.ticks % 10) + delta) / 10.0f;
            f = MathHelper.sin(f * (float)Math.PI * 2.0f) * 0.2f + 0.8f;
            int n = (int)(255.0f * f);
            this.drawTextWithShadow(this.textRenderer, "Saving level..", 8, this.height - 16, n << 16 | n << 8 | n);
        }
        this.drawCenteredTextWithShadow(this.textRenderer, "Game menu", this.width / 2, 40, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}

