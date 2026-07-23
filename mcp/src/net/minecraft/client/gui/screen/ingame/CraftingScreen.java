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
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.screen.CraftingScreenHandler;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class CraftingScreen
extends HandledScreen {
    public CraftingScreen(PlayerInventory inventory, World world, int x, int y, int z) {
        super(new CraftingScreenHandler(inventory, world, x, y, z));
    }

    public void removed() {
        super.removed();
        this.container.onClosed(this.minecraft.player);
    }

    protected void drawForeground() {
        this.textRenderer.draw("Crafting", 28, 6, 0x404040);
        this.textRenderer.draw("Inventory", 8, this.backgroundHeight - 96 + 2, 0x404040);
    }

    protected void drawBackground(float tickDelta) {
        int n = this.minecraft.textureManager.getTextureId("/gui/crafting.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n);
        int n2 = (this.width - this.backgroundWidth) / 2;
        int n3 = (this.height - this.backgroundHeight) / 2;
        this.drawTexture(n2, n3, 0, 0, this.backgroundWidth, this.backgroundHeight);
    }
}

