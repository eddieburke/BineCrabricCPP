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
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.inventory.Inventory;
import net.minecraft.screen.GenericContainerScreenHandler;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class DoubleChestScreen
extends HandledScreen {
    private Inventory playerInventory;
    private Inventory inventory;
    private int rows = 0;

    public DoubleChestScreen(Inventory playerInventory, Inventory inventory) {
        super(new GenericContainerScreenHandler(playerInventory, inventory));
        this.playerInventory = playerInventory;
        this.inventory = inventory;
        this.passEvents = false;
        int n = 222;
        int n2 = n - 108;
        this.rows = inventory.size() / 9;
        this.backgroundHeight = n2 + this.rows * 18;
    }

    protected void drawForeground() {
        this.textRenderer.draw(this.inventory.getName(), 8, 6, 0x404040);
        this.textRenderer.draw(this.playerInventory.getName(), 8, this.backgroundHeight - 96 + 2, 0x404040);
    }

    protected void drawBackground(float tickDelta) {
        int n = this.minecraft.textureManager.getTextureId("/gui/container.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n);
        int n2 = (this.width - this.backgroundWidth) / 2;
        int n3 = (this.height - this.backgroundHeight) / 2;
        this.drawTexture(n2, n3, 0, 0, this.backgroundWidth, this.rows * 18 + 17);
        this.drawTexture(n2, n3 + this.rows * 18 + 17, 0, 126, this.backgroundWidth, 96);
    }
}

