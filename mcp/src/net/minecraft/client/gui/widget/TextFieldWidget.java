/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.widget;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.util.CharacterUtils;

@Environment(value=EnvType.CLIENT)
public class TextFieldWidget
extends DrawContext {
    private final TextRenderer textRenderer;
    private final int x;
    private final int y;
    private final int width;
    private final int height;
    private String text;
    private int maxLength;
    private int focusedTicks;
    public boolean focused = false;
    public boolean enabled = true;
    private Screen parent;

    public TextFieldWidget(Screen parent, TextRenderer textRenderer, int x, int y, int width, int height, String text) {
        this.parent = parent;
        this.textRenderer = textRenderer;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.setText(text);
    }

    public void setText(String text) {
        this.text = text;
    }

    public String getText() {
        return this.text;
    }

    public void tick() {
        ++this.focusedTicks;
    }

    public void keyPressed(char character, int keyCode) {
        if (!this.enabled || !this.focused) {
            return;
        }
        if (character == '\t') {
            this.parent.handleTab();
        }
        if (character == '\u0016') {
            int n;
            String string = Screen.getClipboard();
            if (string == null) {
                string = "";
            }
            if ((n = 32 - this.text.length()) > string.length()) {
                n = string.length();
            }
            if (n > 0) {
                this.text = this.text + string.substring(0, n);
            }
        }
        if (keyCode == 14 && this.text.length() > 0) {
            this.text = this.text.substring(0, this.text.length() - 1);
        }
        if (CharacterUtils.VALID_CHARACTERS.indexOf(character) >= 0 && (this.text.length() < this.maxLength || this.maxLength == 0)) {
            this.text = this.text + character;
        }
    }

    public void mouseClicked(int mouseX, int mouseY, int button) {
        boolean bl = this.enabled && mouseX >= this.x && mouseX < this.x + this.width && mouseY >= this.y && mouseY < this.y + this.height;
        this.setFocused(bl);
    }

    public void setFocused(boolean focused) {
        if (focused && !this.focused) {
            this.focusedTicks = 0;
        }
        this.focused = focused;
    }

    public void render() {
        this.fill(this.x - 1, this.y - 1, this.x + this.width + 1, this.y + this.height + 1, -6250336);
        this.fill(this.x, this.y, this.x + this.width, this.y + this.height, -16777216);
        if (this.enabled) {
            boolean bl = this.focused && this.focusedTicks / 6 % 2 == 0;
            this.drawTextWithShadow(this.textRenderer, this.text + (bl ? "_" : ""), this.x + 4, this.y + (this.height - 8) / 2, 0xE0E0E0);
        } else {
            this.drawTextWithShadow(this.textRenderer, this.text, this.x + 4, this.y + (this.height - 8) / 2, 0x707070);
        }
    }

    public void setMaxLength(int maxLength) {
        this.maxLength = maxLength;
    }
}

