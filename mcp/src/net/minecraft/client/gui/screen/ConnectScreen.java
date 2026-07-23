/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import java.net.ConnectException;
import java.net.UnknownHostException;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screen.DisconnectedScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.network.packet.handshake.HandshakePacket;

@Environment(value=EnvType.CLIENT)
public class ConnectScreen
extends Screen {
    private ClientNetworkHandler networkHandler;
    private boolean connectingCancelled = false;

    public ConnectScreen(final Minecraft minecraft, final String address, final int port) {
        System.out.println("Connecting to " + address + ", " + port);
        minecraft.setWorld(null);
        new Thread(){

            public void run() {
                try {
                    ConnectScreen.this.networkHandler = new ClientNetworkHandler(minecraft, address, port);
                    if (ConnectScreen.this.connectingCancelled) {
                        return;
                    }
                    ConnectScreen.this.networkHandler.sendPacket(new HandshakePacket(minecraft.session.username));
                }
                catch (UnknownHostException unknownHostException) {
                    if (ConnectScreen.this.connectingCancelled) {
                        return;
                    }
                    minecraft.setScreen(new DisconnectedScreen("connect.failed", "disconnect.genericReason", "Unknown host '" + address + "'"));
                }
                catch (ConnectException connectException) {
                    if (ConnectScreen.this.connectingCancelled) {
                        return;
                    }
                    minecraft.setScreen(new DisconnectedScreen("connect.failed", "disconnect.genericReason", connectException.getMessage()));
                }
                catch (Exception exception) {
                    if (ConnectScreen.this.connectingCancelled) {
                        return;
                    }
                    exception.printStackTrace();
                    minecraft.setScreen(new DisconnectedScreen("connect.failed", "disconnect.genericReason", exception.toString()));
                }
            }
        }.start();
    }

    public void tick() {
        if (this.networkHandler != null) {
            this.networkHandler.tick();
        }
    }

    protected void keyPressed(char character, int keyCode) {
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.buttons.clear();
        this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, this.height / 4 + 120 + 12, translationStorage.get("gui.cancel")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            this.connectingCancelled = true;
            if (this.networkHandler != null) {
                this.networkHandler.disconnect();
            }
            this.minecraft.setScreen(new TitleScreen());
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        if (this.networkHandler == null) {
            this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("connect.connecting"), this.width / 2, this.height / 2 - 50, 0xFFFFFF);
            this.drawCenteredTextWithShadow(this.textRenderer, "", this.width / 2, this.height / 2 - 10, 0xFFFFFF);
        } else {
            this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("connect.authorizing"), this.width / 2, this.height / 2 - 50, 0xFFFFFF);
            this.drawCenteredTextWithShadow(this.textRenderer, this.networkHandler.message, this.width / 2, this.height / 2 - 10, 0xFFFFFF);
        }
        super.render(mouseX, mouseY, delta);
    }
}

