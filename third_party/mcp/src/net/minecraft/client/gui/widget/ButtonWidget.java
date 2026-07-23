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
import net.minecraft.client.Minecraft;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ButtonWidget
extends DrawContext {
    protected int width = 200;
    protected int height = 20;
    public int x;
    public int y;
    public String text;
    public int id;
    public boolean active = true;
    public boolean visible = true;

    public ButtonWidget(int id, int x, int y, String text) {
        this(id, x, y, 200, 20, text);
    }

    public ButtonWidget(int id, int x, int y, int width, int height, String text) {
        this.id = id;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.text = text;
    }

    protected int getYImage(boolean hovered) {
        int n = 1;
        if (!this.active) {
            n = 0;
        } else if (hovered) {
            n = 2;
        }
        return n;
    }

    public void render(Minecraft minecraft, int mouseX, int mouseY) {
        if (!this.visible) {
            return;
        }
        TextRenderer textRenderer = minecraft.textRenderer;
        GL11.glBindTexture(3553, minecraft.textureManager.getTextureId("/gui/gui.png"));
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        boolean bl = mouseX >= this.x && mouseY >= this.y && mouseX < this.x + this.width && mouseY < this.y + this.height;
        int n = this.getYImage(bl);
        this.drawTexture(this.x, this.y, 0, 46 + n * 20, this.width / 2, this.height);
        this.drawTexture(this.x + this.width / 2, this.y, 200 - this.width / 2, 46 + n * 20, this.width / 2, this.height);
        this.renderBackground(minecraft, mouseX, mouseY);
        if (!this.active) {
            this.drawCenteredTextWithShadow(textRenderer, this.text, this.x + this.width / 2, this.y + (this.height - 8) / 2, -6250336);
        } else if (bl) {
            this.drawCenteredTextWithShadow(textRenderer, this.text, this.x + this.width / 2, this.y + (this.height - 8) / 2, 0xFFFFA0);
        } else {
            this.drawCenteredTextWithShadow(textRenderer, this.text, this.x + this.width / 2, this.y + (this.height - 8) / 2, 0xE0E0E0);
        }
    }

    protected void renderBackground(Minecraft minecraft, int mouseX, int mouseY) {
    }

    public void mouseReleased(int mouseX, int mouseY) {
    }

    public boolean isMouseOver(Minecraft minecraft, int mouseX, int mouseY) {
        return this.active && mouseX >= this.x && mouseY >= this.y && mouseX < this.x + this.width && mouseY < this.y + this.height;
    }
}

