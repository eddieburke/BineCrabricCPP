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
import net.minecraft.client.gui.screen.ConnectScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.client.resource.language.TranslationStorage;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class MultiplayerScreen
extends Screen {
    private Screen parent;
    private TextFieldWidget serverField;

    public MultiplayerScreen(Screen parent) {
        this.parent = parent;
    }

    public void tick() {
        this.serverField.tick();
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        Keyboard.enableRepeatEvents(true);
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 96 + 12, translationStorage.get("multiplayer.connect")));
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height / 4 + 120 + 12, translationStorage.get("gui.cancel")));
        String string = this.minecraft.options.lastServer.replaceAll("_", ":");
        ((ButtonWidget)this.buttons.get((int)0)).active = string.length() > 0;
        this.serverField = new TextFieldWidget(this, this.textRenderer, this.width / 2 - 100, this.height / 4 - 10 + 50 + 18, 200, 20, string);
        this.serverField.focused = true;
        this.serverField.setMaxLength(128);
    }

    public void removed() {
        Keyboard.enableRepeatEvents(false);
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) {
            return;
        }
        if (button.id == 1) {
            this.minecraft.setScreen(this.parent);
        } else if (button.id == 0) {
            int n;
            String string = this.serverField.getText().trim();
            this.minecraft.options.lastServer = string.replaceAll(":", "_");
            this.minecraft.options.save();
            String[] stringArray = string.split(":");
            if (string.startsWith("[") && (n = string.indexOf("]")) > 0) {
                String string2 = string.substring(1, n);
                String string3 = string.substring(n + 1).trim();
                if (string3.startsWith(":") && string3.length() > 0) {
                    string3 = string3.substring(1);
                    stringArray = new String[]{string2, string3};
                } else {
                    stringArray = new String[]{string2};
                }
            }
            if (stringArray.length > 2) {
                stringArray = new String[]{string};
            }
            this.minecraft.setScreen(new ConnectScreen(this.minecraft, stringArray[0], stringArray.length > 1 ? this.parseInt(stringArray[1], 25565) : 25565));
        }
    }

    private int parseInt(String s, int defaultValue) {
        try {
            return Integer.parseInt(s.trim());
        }
        catch (Exception exception) {
            return defaultValue;
        }
    }

    protected void keyPressed(char character, int keyCode) {
        this.serverField.keyPressed(character, keyCode);
        if (character == '\r') {
            this.buttonClicked((ButtonWidget)this.buttons.get(0));
        }
        ((ButtonWidget)this.buttons.get((int)0)).active = this.serverField.getText().length() > 0;
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        super.mouseClicked(mouseX, mouseY, button);
        this.serverField.mouseClicked(mouseX, mouseY, button);
    }

    public void render(int mouseX, int mouseY, float delta) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("multiplayer.title"), this.width / 2, this.height / 4 - 60 + 20, 0xFFFFFF);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("multiplayer.info1"), this.width / 2 - 140, this.height / 4 - 60 + 60 + 0, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("multiplayer.info2"), this.width / 2 - 140, this.height / 4 - 60 + 60 + 9, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("multiplayer.ipinfo"), this.width / 2 - 140, this.height / 4 - 60 + 60 + 36, 0xA0A0A0);
        this.serverField.render();
        super.render(mouseX, mouseY, delta);
    }
}

