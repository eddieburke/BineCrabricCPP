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

@Environment(value=EnvType.CLIENT)
public class FatalErrorScreen
extends Screen {
    private String title;
    private String description;

    public void init() {
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.fillGradient(0, 0, this.width, this.height, -12574688, -11530224);
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 90, 0xFFFFFF);
        this.drawCenteredTextWithShadow(this.textRenderer, this.description, this.width / 2, 110, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }

    protected void keyPressed(char character, int keyCode) {
    }
}

