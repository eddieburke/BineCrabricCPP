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
import net.minecraft.util.CharacterUtils;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class ChatScreen
extends Screen {
    protected String text = "";
    private int focusedTicks = 0;
    private static final String VALID_CHARACTERS = CharacterUtils.VALID_CHARACTERS;

    public void init() {
        Keyboard.enableRepeatEvents(true);
    }

    public void removed() {
        Keyboard.enableRepeatEvents(false);
    }

    public void tick() {
        ++this.focusedTicks;
    }

    protected void keyPressed(char character, int keyCode) {
        if (keyCode == Keyboard.KEY_ESCAPE) {
            this.minecraft.setScreen(null);
            return;
        }
        if (keyCode == Keyboard.KEY_RETURN) {
            String string;
            String string2 = this.text.trim();
            if (string2.length() > 0 && !this.minecraft.isCommand(string = this.text.trim())) {
                this.minecraft.player.sendChatMessage(string);
            }
            this.minecraft.setScreen(null);
            return;
        }
        if (keyCode == Keyboard.KEY_BACK && this.text.length() > 0) {
            this.text = this.text.substring(0, this.text.length() - 1);
        }
        if (VALID_CHARACTERS.indexOf(character) >= 0 && this.text.length() < 100) {
            this.text = this.text + character;
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.fill(2, this.height - 14, this.width - 2, this.height - 2, Integer.MIN_VALUE);
        this.drawTextWithShadow(this.textRenderer, "> " + this.text + (this.focusedTicks / 6 % 2 == 0 ? "_" : ""), 4, this.height - 12, 0xE0E0E0);
        super.render(mouseX, mouseY, delta);
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        if (button == 0) {
            if (this.minecraft.inGameHud.selectedName != null) {
                if (this.text.length() > 0 && !this.text.endsWith(" ")) {
                    this.text = this.text + " ";
                }
                this.text = this.text + this.minecraft.inGameHud.selectedName;
                int n = 100;
                if (this.text.length() > n) {
                    this.text = this.text.substring(0, n);
                }
            } else {
                super.mouseClicked(mouseX, mouseY, button);
            }
        }
    }
}

