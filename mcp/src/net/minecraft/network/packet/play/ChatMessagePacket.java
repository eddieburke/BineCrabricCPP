/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class ChatMessagePacket
extends Packet {
    public String chatMessage;

    public ChatMessagePacket() {
    }

    public ChatMessagePacket(String chatMessage) {
        if (chatMessage.length() > 119) {
            chatMessage = chatMessage.substring(0, 119);
        }
        this.chatMessage = chatMessage;
    }

    public void read(DataInputStream stream) {
        this.chatMessage = ChatMessagePacket.readString(stream, 119);
    }

    public void write(DataOutputStream stream) {
        ChatMessagePacket.writeString(this.chatMessage, stream);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onChatMessage(this);
    }

    public int size() {
        return this.chatMessage.length();
    }
}

