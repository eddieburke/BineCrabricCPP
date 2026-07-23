/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class DisconnectPacket
extends Packet {
    public String reason;

    public DisconnectPacket() {
    }

    public DisconnectPacket(String reason) {
        this.reason = reason;
    }

    public void read(DataInputStream stream) {
        this.reason = DisconnectPacket.readString(stream, 100);
    }

    public void write(DataOutputStream stream) {
        DisconnectPacket.writeString(this.reason, stream);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onDisconnect(this);
    }

    public int size() {
        return this.reason.length();
    }
}

