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
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.option.Option;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class SliderWidget
extends ButtonWidget {
    public float value = 1.0f;
    public boolean dragging = false;
    private Option option = null;

    public SliderWidget(int id, int x, int y, Option option, String message, float value) {
        super(id, x, y, 150, 20, message);
        this.option = option;
        this.value = value;
    }

    protected int getYImage(boolean hovered) {
        return 0;
    }

    protected void renderBackground(Minecraft minecraft, int mouseX, int mouseY) {
        if (!this.visible) {
            return;
        }
        if (this.dragging) {
            this.value = (float)(mouseX - (this.x + 4)) / (float)(this.width - 8);
            if (this.value < 0.0f) {
                this.value = 0.0f;
            }
            if (this.value > 1.0f) {
                this.value = 1.0f;
            }
            minecraft.options.setFloat(this.option, this.value);
            this.text = minecraft.options.getString(this.option);
        }
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.drawTexture(this.x + (int)(this.value * (float)(this.width - 8)), this.y, 0, 66, 4, 20);
        this.drawTexture(this.x + (int)(this.value * (float)(this.width - 8)) + 4, this.y, 196, 66, 4, 20);
    }

    public boolean isMouseOver(Minecraft minecraft, int mouseX, int mouseY) {
        if (super.isMouseOver(minecraft, mouseX, mouseY)) {
            this.value = (float)(mouseX - (this.x + 4)) / (float)(this.width - 8);
            if (this.value < 0.0f) {
                this.value = 0.0f;
            }
            if (this.value > 1.0f) {
                this.value = 1.0f;
            }
            minecraft.options.setFloat(this.option, this.value);
            this.text = minecraft.options.getString(this.option);
            this.dragging = true;
            return true;
        }
        return false;
    }

    public void mouseReleased(int mouseX, int mouseY) {
        this.dragging = false;
    }
}

