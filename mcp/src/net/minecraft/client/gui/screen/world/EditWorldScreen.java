/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.world;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.storage.WorldStorageSource;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class EditWorldScreen
extends Screen {
    private Screen parent;
    private TextFieldWidget levelNameTextField;
    private final String worldName;

    public EditWorldScreen(Screen parent, String worldName) {
        this.parent = parent;
        this.worldName = worldName;
    }

    public void tick() {
        this.levelNameTextField.tick();
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        Keyboard.enableRepeatEvents(true);
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 96 + 12, translationStorage.get("selectWorld.renameButton")));
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height / 4 + 120 + 12, translationStorage.get("gui.cancel")));
        WorldStorageSource worldStorageSource = this.minecraft.getWorldStorageSource();
        WorldProperties worldProperties = worldStorageSource.getWorldProperties(this.worldName);
        String string = worldProperties.getName();
        this.levelNameTextField = new TextFieldWidget(this, this.textRenderer, this.width / 2 - 100, 60, 200, 20, string);
        this.levelNameTextField.focused = true;
        this.levelNameTextField.setMaxLength(32);
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
            WorldStorageSource worldStorageSource = this.minecraft.getWorldStorageSource();
            worldStorageSource.rename(this.worldName, this.levelNameTextField.getText().trim());
            this.minecraft.setScreen(this.parent);
        }
    }

    protected void keyPressed(char character, int keyCode) {
        this.levelNameTextField.keyPressed(character, keyCode);
        boolean bl = ((ButtonWidget)this.buttons.get((int)0)).active = this.levelNameTextField.getText().trim().length() > 0;
        if (character == '\r') {
            this.buttonClicked((ButtonWidget)this.buttons.get(0));
        }
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        super.mouseClicked(mouseX, mouseY, button);
        this.levelNameTextField.mouseClicked(mouseX, mouseY, button);
    }

    public void render(int mouseX, int mouseY, float delta) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.renameTitle"), this.width / 2, this.height / 4 - 60 + 20, 0xFFFFFF);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.enterName"), this.width / 2 - 100, 47, 0xA0A0A0);
        this.levelNameTextField.render();
        super.render(mouseX, mouseY, delta);
    }
}

