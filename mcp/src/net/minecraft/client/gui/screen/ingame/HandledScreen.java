/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.ingame;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;
import org.lwjgl.input.Keyboard;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public abstract class HandledScreen
extends Screen {
    private static ItemRenderer itemRenderer = new ItemRenderer();
    protected int backgroundWidth = 176;
    protected int backgroundHeight = 166;
    public ScreenHandler container;

    public HandledScreen(ScreenHandler container) {
        this.container = container;
    }

    public void init() {
        super.init();
        this.minecraft.player.currentScreenHandler = this.container;
    }

    public void render(int mouseX, int mouseY, float delta) {
        int n;
        int n2;
        Object object;
        this.renderBackground();
        int n3 = (this.width - this.backgroundWidth) / 2;
        int n4 = (this.height - this.backgroundHeight) / 2;
        this.drawBackground(delta);
        GL11.glPushMatrix();
        GL11.glRotatef(120.0f, 1.0f, 0.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        GL11.glPushMatrix();
        GL11.glTranslatef(n3, n4, 0.0f);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glEnable(32826);
        Slot slot = null;
        for (int i = 0; i < this.container.slots.size(); ++i) {
            object = (Slot)this.container.slots.get(i);
            if (this.isPointOverSlot((Slot)object, mouseX, mouseY)) {
                slot = object;
                GL11.glDisable(2896);
                GL11.glDisable(2929);
                n2 = ((Slot)object).x;
                n = ((Slot)object).y;
                this.fillGradient(n2, n, n2 + 16, n + 16, -2130706433, -2130706433);
                GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                GL11.glEnable(2896);
                GL11.glEnable(2929);
            }
            this.drawSlot((Slot)object);
        }
        PlayerInventory playerInventory = this.minecraft.player.inventory;
        if (playerInventory.getCursorStack() != null) {
            GL11.glTranslatef(0.0f, 0.0f, 32.0f);
            itemRenderer.renderGuiItem(this.textRenderer, this.minecraft.textureManager, playerInventory.getCursorStack(), mouseX - n3 - 8, mouseY - n4 - 8);
            itemRenderer.renderGuiItemDecoration(this.textRenderer, this.minecraft.textureManager, playerInventory.getCursorStack(), mouseX - n3 - 8, mouseY - n4 - 8);
        }
        GL11.glDisable(32826);
        Lighting.turnOff();
        GL11.glDisable(2896);
        GL11.glDisable(2929);
        this.drawForeground();
        if (playerInventory.getCursorStack() == null && slot != null && slot.hasStack() && ((String)(object = ("" + TranslationStorage.getInstance().getClientTranslation(slot.getStack().getTranslationKey())).trim())).length() > 0) {
            n2 = mouseX - n3 + 12;
            n = mouseY - n4 - 12;
            int n5 = this.textRenderer.getWidth((String)object);
            this.fillGradient(n2 - 3, n - 3, n2 + n5 + 3, n + 8 + 3, -1073741824, -1073741824);
            this.textRenderer.drawWithShadow((String)object, n2, n, -1);
        }
        GL11.glPopMatrix();
        super.render(mouseX, mouseY, delta);
        GL11.glEnable(2896);
        GL11.glEnable(2929);
    }

    protected void drawForeground() {
    }

    protected abstract void drawBackground(float var1);

    private void drawSlot(Slot slot) {
        int n;
        int n2 = slot.x;
        int n3 = slot.y;
        ItemStack itemStack = slot.getStack();
        if (itemStack == null && (n = slot.getBackgroundTextureId()) >= 0) {
            GL11.glDisable(2896);
            this.minecraft.textureManager.bindTexture(this.minecraft.textureManager.getTextureId("/gui/items.png"));
            this.drawTexture(n2, n3, n % 16 * 16, n / 16 * 16, 16, 16);
            GL11.glEnable(2896);
            return;
        }
        itemRenderer.renderGuiItem(this.textRenderer, this.minecraft.textureManager, itemStack, n2, n3);
        itemRenderer.renderGuiItemDecoration(this.textRenderer, this.minecraft.textureManager, itemStack, n2, n3);
    }

    private Slot getSlotAt(int x, int y) {
        for (int i = 0; i < this.container.slots.size(); ++i) {
            Slot slot = (Slot)this.container.slots.get(i);
            if (!this.isPointOverSlot(slot, x, y)) continue;
            return slot;
        }
        return null;
    }

    private boolean isPointOverSlot(Slot slot, int x, int y) {
        int n = (this.width - this.backgroundWidth) / 2;
        int n2 = (this.height - this.backgroundHeight) / 2;
        return (x -= n) >= slot.x - 1 && x < slot.x + 16 + 1 && (y -= n2) >= slot.y - 1 && y < slot.y + 16 + 1;
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        super.mouseClicked(mouseX, mouseY, button);
        if (button == 0 || button == 1) {
            Slot slot = this.getSlotAt(mouseX, mouseY);
            int n = (this.width - this.backgroundWidth) / 2;
            int n2 = (this.height - this.backgroundHeight) / 2;
            boolean bl = mouseX < n || mouseY < n2 || mouseX >= n + this.backgroundWidth || mouseY >= n2 + this.backgroundHeight;
            int n3 = -1;
            if (slot != null) {
                n3 = slot.id;
            }
            if (bl) {
                n3 = -999;
            }
            if (n3 != -1) {
                boolean bl2 = n3 != -999 && (Keyboard.isKeyDown(42) || Keyboard.isKeyDown(54));
                this.minecraft.interactionManager.clickSlot(this.container.syncId, n3, button, bl2, this.minecraft.player);
            }
        }
    }

    protected void mouseReleased(int mouseX, int mouseY, int button) {
        if (button == 0) {
            // empty if block
        }
    }

    protected void keyPressed(char character, int keyCode) {
        if (keyCode == Keyboard.KEY_ESCAPE || keyCode == this.minecraft.options.inventoryKey.code) {
            this.minecraft.player.closeHandledScreen();
        }
    }

    public void removed() {
        if (this.minecraft.player == null) {
            return;
        }
        this.minecraft.interactionManager.onScreenRemoved(this.container.syncId, this.minecraft.player);
    }

    public boolean shouldPause() {
        return false;
    }

    public void tick() {
        super.tick();
        if (!this.minecraft.player.isAlive() || this.minecraft.player.dead) {
            this.minecraft.player.closeHandledScreen();
        }
    }
}
