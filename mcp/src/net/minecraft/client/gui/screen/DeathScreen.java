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
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class DeathScreen
extends Screen {
    public void init() {
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height / 4 + 72, "Respawn"));
        this.buttons.add(new ButtonWidget(2, this.width / 2 - 100, this.height / 4 + 96, "Title menu"));
        if (this.minecraft.session == null) {
            ((ButtonWidget)this.buttons.get((int)1)).active = false;
        }
    }

    protected void keyPressed(char character, int keyCode) {
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            // empty if block
        }
        if (button.id == 1) {
            this.minecraft.player.respawn();
            this.minecraft.setScreen(null);
        }
        if (button.id == 2) {
            this.minecraft.setWorld(null);
            this.minecraft.setScreen(new TitleScreen());
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.fillGradient(0, 0, this.width, this.height, 0x60500000, -1602211792);
        GL11.glPushMatrix();
        GL11.glScalef(2.0f, 2.0f, 2.0f);
        this.drawCenteredTextWithShadow(this.textRenderer, "Game over!", this.width / 2 / 2, 30, 0xFFFFFF);
        GL11.glPopMatrix();
        this.drawCenteredTextWithShadow(this.textRenderer, "Score: &e" + this.minecraft.player.getScore(), this.width / 2, 100, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }

    public boolean shouldPause() {
        return false;
    }
}

