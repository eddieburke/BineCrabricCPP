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
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.screen.FurnaceScreenHandler;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class FurnaceScreen
extends HandledScreen {
    private FurnaceBlockEntity furnaceBlockEntity;

    public FurnaceScreen(PlayerInventory inventory, FurnaceBlockEntity furnaceBlockEntity) {
        super(new FurnaceScreenHandler(inventory, furnaceBlockEntity));
        this.furnaceBlockEntity = furnaceBlockEntity;
    }

    protected void drawForeground() {
        this.textRenderer.draw("Furnace", 60, 6, 0x404040);
        this.textRenderer.draw("Inventory", 8, this.backgroundHeight - 96 + 2, 0x404040);
    }

    protected void drawBackground(float tickDelta) {
        int n;
        int n2 = this.minecraft.textureManager.getTextureId("/gui/furnace.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n2);
        int n3 = (this.width - this.backgroundWidth) / 2;
        int n4 = (this.height - this.backgroundHeight) / 2;
        this.drawTexture(n3, n4, 0, 0, this.backgroundWidth, this.backgroundHeight);
        if (this.furnaceBlockEntity.isBurning()) {
            n = this.furnaceBlockEntity.getFuelTimeDelta(12);
            this.drawTexture(n3 + 56, n4 + 36 + 12 - n, 176, 12 - n, 14, n + 2);
        }
        n = this.furnaceBlockEntity.getCookTimeDelta(24);
        this.drawTexture(n3 + 79, n4 + 34, 176, 14, n + 1, 16);
    }
}

