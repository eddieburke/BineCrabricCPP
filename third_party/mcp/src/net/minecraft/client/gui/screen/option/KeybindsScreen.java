/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.OptionButtonWidget;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public class KeybindsScreen
extends Screen {
    private Screen parent;
    protected String title = "Controls";
    private GameOptions gameOptions;
    private int selectedKeyBinding = -1;

    public KeybindsScreen(Screen parent, GameOptions gameOptions) {
        this.parent = parent;
        this.gameOptions = gameOptions;
    }

    private int getControlsListX() {
        return this.width / 2 - 155;
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        int n = this.getControlsListX();
        for (int i = 0; i < this.gameOptions.allKeys.length; ++i) {
            this.buttons.add(new OptionButtonWidget(i, n + i % 2 * 160, this.height / 6 + 24 * (i >> 1), 70, 20, this.gameOptions.getKeybindKey(i)));
        }
        this.buttons.add(new ButtonWidget(200, this.width / 2 - 100, this.height / 6 + 168, translationStorage.get("gui.done")));
        this.title = translationStorage.get("controls.title");
    }

    protected void buttonClicked(ButtonWidget button) {
        for (int i = 0; i < this.gameOptions.allKeys.length; ++i) {
            ((ButtonWidget)this.buttons.get((int)i)).text = this.gameOptions.getKeybindKey(i);
        }
        if (button.id == 200) {
            this.minecraft.setScreen(this.parent);
        } else {
            this.selectedKeyBinding = button.id;
            button.text = "> " + this.gameOptions.getKeybindKey(button.id) + " <";
        }
    }

    protected void keyPressed(char character, int keyCode) {
        if (this.selectedKeyBinding >= 0) {
            this.gameOptions.setKeybindKey(this.selectedKeyBinding, keyCode);
            ((ButtonWidget)this.buttons.get((int)this.selectedKeyBinding)).text = this.gameOptions.getKeybindKey(this.selectedKeyBinding);
            this.selectedKeyBinding = -1;
        } else {
            super.keyPressed(character, keyCode);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        int n = this.getControlsListX();
        for (int i = 0; i < this.gameOptions.allKeys.length; ++i) {
            this.drawTextWithShadow(this.textRenderer, this.gameOptions.getKeybindName(i), n + i % 2 * 160 + 70 + 6, this.height / 6 + 24 * (i >> 1) + 7, -1);
        }
        super.render(mouseX, mouseY, delta);
    }
}

