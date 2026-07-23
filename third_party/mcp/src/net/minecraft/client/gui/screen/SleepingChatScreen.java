/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 *  net.fabricmc.yarn.constants.network.ClientCommandMode
 */
package net.minecraft.client.gui.screen;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.yarn.constants.network.ClientCommandMode;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.network.MultiplayerClientPlayerEntity;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class SleepingChatScreen
extends ChatScreen {
    public void init() {
        Keyboard.enableRepeatEvents(true);
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, this.height - 40, translationStorage.get("multiplayer.stopSleeping")));
    }

    public void removed() {
        Keyboard.enableRepeatEvents(false);
    }

    protected void keyPressed(char character, int keyCode) {
        if (keyCode == Keyboard.KEY_ESCAPE) {
            this.stopSleeping();
        } else if (keyCode == Keyboard.KEY_RETURN) {
            String string = this.text.trim();
            if (string.length() > 0) {
                this.minecraft.player.sendChatMessage(this.text.trim());
            }
            this.text = "";
        } else {
            super.keyPressed(character, keyCode);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        super.render(mouseX, mouseY, delta);
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 1) {
            this.stopSleeping();
        } else {
            super.buttonClicked(button);
        }
    }

    private void stopSleeping() {
        if (this.minecraft.player instanceof MultiplayerClientPlayerEntity) {
            ClientNetworkHandler clientNetworkHandler = ((MultiplayerClientPlayerEntity)this.minecraft.player).networkHandler;
            clientNetworkHandler.sendPacket(new ClientCommandC2SPacket(this.minecraft.player, ClientCommandMode.STOP_SLEEPING));
        }
    }
}

