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
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.network.packet.play.KeepAlivePacket;

@Environment(value=EnvType.CLIENT)
public class DownloadingTerrainScreen
extends Screen {
    private ClientNetworkHandler networkHandler;
    private int ticks = 0;

    public DownloadingTerrainScreen(ClientNetworkHandler networkHandler) {
        this.networkHandler = networkHandler;
    }

    protected void keyPressed(char character, int keyCode) {
    }

    public void init() {
        this.buttons.clear();
    }

    public void tick() {
        ++this.ticks;
        if (this.ticks % 20 == 0) {
            this.networkHandler.sendPacket(new KeepAlivePacket());
        }
        if (this.networkHandler != null) {
            this.networkHandler.tick();
        }
    }

    protected void buttonClicked(ButtonWidget button) {
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackgroundTexture(0);
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("multiplayer.downloadingTerrain"), this.width / 2, this.height / 2 - 50, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}

