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
import net.minecraft.client.gui.widget.ButtonWidget;

@Environment(value=EnvType.CLIENT)
public class OutOfMemoryScreen
extends Screen {
    private int ticksRan = 0;

    public void tick() {
        ++this.ticksRan;
    }

    public void init() {
    }

    protected void buttonClicked(ButtonWidget button) {
    }

    protected void keyPressed(char character, int keyCode) {
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, "Out of memory!", this.width / 2, this.height / 4 - 60 + 20, 0xFFFFFF);
        this.drawTextWithShadow(this.textRenderer, "Minecraft has run out of memory.", this.width / 2 - 140, this.height / 4 - 60 + 60 + 0, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "This could be caused by a bug in the game or by the", this.width / 2 - 140, this.height / 4 - 60 + 60 + 18, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "Java Virtual Machine not being allocated enough", this.width / 2 - 140, this.height / 4 - 60 + 60 + 27, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "memory. If you are playing in a web browser, try", this.width / 2 - 140, this.height / 4 - 60 + 60 + 36, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "downloading the game and playing it offline.", this.width / 2 - 140, this.height / 4 - 60 + 60 + 45, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "To prevent level corruption, the current game has quit.", this.width / 2 - 140, this.height / 4 - 60 + 60 + 63, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, "Please restart the game.", this.width / 2 - 140, this.height / 4 - 60 + 60 + 81, 0xA0A0A0);
        super.render(mouseX, mouseY, delta);
    }
}

