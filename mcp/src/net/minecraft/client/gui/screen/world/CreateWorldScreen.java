/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.world;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.SingleplayerInteractionManager;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.util.CharacterUtils;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.storage.WorldStorageSource;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class CreateWorldScreen
extends Screen {
    private Screen parent;
    private TextFieldWidget worldNameField;
    private TextFieldWidget seedField;
    private String worldSaveName;
    private boolean creatingLevel;

    public CreateWorldScreen(Screen parent) {
        this.parent = parent;
    }

    public void tick() {
        this.worldNameField.tick();
        this.seedField.tick();
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        Keyboard.enableRepeatEvents(true);
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 96 + 12, translationStorage.get("selectWorld.create")));
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height / 4 + 120 + 12, translationStorage.get("gui.cancel")));
        this.worldNameField = new TextFieldWidget(this, this.textRenderer, this.width / 2 - 100, 60, 200, 20, translationStorage.get("selectWorld.newWorld"));
        this.worldNameField.focused = true;
        this.worldNameField.setMaxLength(32);
        this.seedField = new TextFieldWidget(this, this.textRenderer, this.width / 2 - 100, 116, 200, 20, "");
        this.getSaveDirectoryNames();
    }

    private void getSaveDirectoryNames() {
        this.worldSaveName = this.worldNameField.getText().trim();
        for (char c : CharacterUtils.INVALID_CHARS_WORLD_NAME) {
            this.worldSaveName = this.worldSaveName.replace(c, '_');
        }
        if (MathHelper.isNullOrEmpty(this.worldSaveName)) {
            this.worldSaveName = "World";
        }
        this.worldSaveName = CreateWorldScreen.getWorldSaveName(this.minecraft.getWorldStorageSource(), this.worldSaveName);
    }

    public static String getWorldSaveName(WorldStorageSource storageSource, String worldName) {
        while (storageSource.getWorldProperties(worldName) != null) {
            worldName = worldName + "-";
        }
        return worldName;
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
            this.minecraft.setScreen(null);
            if (this.creatingLevel) {
                return;
            }
            this.creatingLevel = true;
            long l = new Random().nextLong();
            String string = this.seedField.getText();
            if (!MathHelper.isNullOrEmpty(string)) {
                try {
                    long l2 = Long.parseLong(string);
                    if (l2 != 0L) {
                        l = l2;
                    }
                }
                catch (NumberFormatException numberFormatException) {
                    l = string.hashCode();
                }
            }
            this.minecraft.interactionManager = new SingleplayerInteractionManager(this.minecraft);
            this.minecraft.startGame(this.worldSaveName, this.worldNameField.getText(), l);
            this.minecraft.setScreen(null);
        }
    }

    protected void keyPressed(char character, int keyCode) {
        if (this.worldNameField.focused) {
            this.worldNameField.keyPressed(character, keyCode);
        } else {
            this.seedField.keyPressed(character, keyCode);
        }
        if (character == '\r') {
            this.buttonClicked((ButtonWidget)this.buttons.get(0));
        }
        ((ButtonWidget)this.buttons.get((int)0)).active = this.worldNameField.getText().length() > 0;
        this.getSaveDirectoryNames();
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        super.mouseClicked(mouseX, mouseY, button);
        this.worldNameField.mouseClicked(mouseX, mouseY, button);
        this.seedField.mouseClicked(mouseX, mouseY, button);
    }

    public void render(int mouseX, int mouseY, float delta) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.create"), this.width / 2, this.height / 4 - 60 + 20, 0xFFFFFF);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.enterName"), this.width / 2 - 100, 47, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.resultFolder") + " " + this.worldSaveName, this.width / 2 - 100, 85, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.enterSeed"), this.width / 2 - 100, 104, 0xA0A0A0);
        this.drawTextWithShadow(this.textRenderer, translationStorage.get("selectWorld.seedInfo"), this.width / 2 - 100, 140, 0xA0A0A0);
        this.worldNameField.render();
        this.seedField.render();
        super.render(mouseX, mouseY, delta);
    }

    public void handleTab() {
        if (this.worldNameField.focused) {
            this.worldNameField.setFocused(false);
            this.seedField.setFocused(true);
        } else {
            this.worldNameField.setFocused(true);
            this.seedField.setFocused(false);
        }
    }
}

