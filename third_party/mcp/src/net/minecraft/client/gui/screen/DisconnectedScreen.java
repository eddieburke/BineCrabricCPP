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
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public class DisconnectedScreen
extends Screen {
    private String title;
    private String reason;

    public DisconnectedScreen(String title, String reason, Object ... args) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.title = translationStorage.get(title);
        this.reason = args != null ? translationStorage.get(reason, args) : translationStorage.get(reason);
    }

    public void tick() {
    }

    protected void keyPressed(char character, int keyCode) {
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 120 + 12, translationStorage.get("gui.toMenu")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            this.minecraft.setScreen(new TitleScreen());
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, this.height / 2 - 50, 0xFFFFFF);
        this.drawCenteredTextWithShadow(this.textRenderer, this.reason, this.width / 2, this.height / 2 - 10, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}

