/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import java.awt.Toolkit;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.ParticlesGui;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.render.Tessellator;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class Screen
extends DrawContext {
    protected Minecraft minecraft;
    public int width;
    public int height;
    protected List buttons = new ArrayList();
    public boolean passEvents = false;
    protected TextRenderer textRenderer;
    public ParticlesGui particlesGui;
    private ButtonWidget selectedButton = null;

    public void render(int mouseX, int mouseY, float delta) {
        for (int i = 0; i < this.buttons.size(); ++i) {
            ButtonWidget buttonWidget = (ButtonWidget)this.buttons.get(i);
            buttonWidget.render(this.minecraft, mouseX, mouseY);
        }
    }

    protected void keyPressed(char character, int keyCode) {
        if (keyCode == Keyboard.KEY_ESCAPE) {
            this.minecraft.setScreen(null);
            this.minecraft.lockMouse();
        }
    }

    public static String getClipboard() {
        try {
            Transferable transferable = Toolkit.getDefaultToolkit().getSystemClipboard().getContents(null);
            if (transferable != null && transferable.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                String string = (String)transferable.getTransferData(DataFlavor.stringFlavor);
                return string;
            }
        }
        catch (Exception exception) {
            // empty catch block
        }
        return null;
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        if (button == 0) {
            for (int i = 0; i < this.buttons.size(); ++i) {
                ButtonWidget buttonWidget = (ButtonWidget)this.buttons.get(i);
                if (!buttonWidget.isMouseOver(this.minecraft, mouseX, mouseY)) continue;
                this.selectedButton = buttonWidget;
                this.minecraft.soundManager.playSound("random.click", 1.0f, 1.0f);
                this.buttonClicked(buttonWidget);
            }
        }
    }

    protected void mouseReleased(int mouseX, int mouseY, int button) {
        if (this.selectedButton != null && button == 0) {
            this.selectedButton.mouseReleased(mouseX, mouseY);
            this.selectedButton = null;
        }
    }

    protected void buttonClicked(ButtonWidget button) {
    }

    public void init(Minecraft minecraft, int width, int height) {
        this.particlesGui = new ParticlesGui(minecraft);
        this.minecraft = minecraft;
        this.textRenderer = minecraft.textRenderer;
        this.width = width;
        this.height = height;
        this.buttons.clear();
        this.init();
    }

    public void init() {
    }

    public void tickInput() {
        while (Mouse.next()) {
            this.onMouseEvent();
        }
        while (Keyboard.next()) {
            this.onKeyboardEvent();
        }
    }

    public void onMouseEvent() {
        if (Mouse.getEventButtonState()) {
            int n = Mouse.getEventX() * this.width / this.minecraft.displayWidth;
            int n2 = this.height - Mouse.getEventY() * this.height / this.minecraft.displayHeight - 1;
            this.mouseClicked(n, n2, Mouse.getEventButton());
        } else {
            int n = Mouse.getEventX() * this.width / this.minecraft.displayWidth;
            int n3 = this.height - Mouse.getEventY() * this.height / this.minecraft.displayHeight - 1;
            this.mouseReleased(n, n3, Mouse.getEventButton());
        }
    }

    public void onKeyboardEvent() {
        if (Keyboard.getEventKeyState()) {
            if (Keyboard.getEventKey() == 87) {
                this.minecraft.toggleFullscreen();
                return;
            }
            this.keyPressed(Keyboard.getEventCharacter(), Keyboard.getEventKey());
        }
    }

    public void tick() {
    }

    public void removed() {
    }

    public void renderBackground() {
        this.renderBackground(0);
    }

    public void renderBackground(int vOffset) {
        if (this.minecraft.world != null) {
            this.fillGradient(0, 0, this.width, this.height, -1072689136, -804253680);
        } else {
            this.renderBackgroundTexture(vOffset);
        }
    }

    public void renderBackgroundTexture(int vOffset) {
        GL11.glDisable(2896);
        GL11.glDisable(2912);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/background.png"));
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        float f = 32.0f;
        tessellator.startQuads();
        tessellator.color(0x404040);
        tessellator.vertex(0.0, this.height, 0.0, 0.0, (float)this.height / f + (float)vOffset);
        tessellator.vertex(this.width, this.height, 0.0, (float)this.width / f, (float)this.height / f + (float)vOffset);
        tessellator.vertex(this.width, 0.0, 0.0, (float)this.width / f, 0 + vOffset);
        tessellator.vertex(0.0, 0.0, 0.0, 0.0, 0 + vOffset);
        tessellator.draw();
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    public boolean shouldPause() {
        return true;
    }

    public void confirmed(boolean confirmed, int id) {
    }

    public void handleTab() {
    }
}

