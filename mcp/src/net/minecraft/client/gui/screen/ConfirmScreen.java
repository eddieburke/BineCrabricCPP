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
import net.minecraft.client.gui.widget.OptionButtonWidget;

@Environment(value=EnvType.CLIENT)
public class ConfirmScreen
extends Screen {
    private Screen parent;
    private String message1;
    private String message2;
    private String confirmText;
    private String cancelText;
    private int id;

    public ConfirmScreen(Screen parent, String message1, String message2, String confirmText, String cancelText, int id) {
        this.parent = parent;
        this.message1 = message1;
        this.message2 = message2;
        this.confirmText = confirmText;
        this.cancelText = cancelText;
        this.id = id;
    }

    public void init() {
        this.buttons.add(new OptionButtonWidget(0, this.width / 2 - 155 + 0, this.height / 6 + 96, this.confirmText));
        this.buttons.add(new OptionButtonWidget(1, this.width / 2 - 155 + 160, this.height / 6 + 96, this.cancelText));
    }

    protected void buttonClicked(ButtonWidget button) {
        this.parent.confirmed(button.id == 0, this.id);
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.message1, this.width / 2, 70, 0xFFFFFF);
        this.drawCenteredTextWithShadow(this.textRenderer, this.message2, this.width / 2, 90, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}

