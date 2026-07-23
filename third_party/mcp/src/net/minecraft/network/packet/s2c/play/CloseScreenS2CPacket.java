/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class CloseScreenS2CPacket
extends Packet {
    public int syncId;

    public CloseScreenS2CPacket() {
    }

    public CloseScreenS2CPacket(int syncId) {
        this.syncId = syncId;
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onCloseScreen(this);
    }

    public void read(DataInputStream stream) {
        this.syncId = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeByte(this.syncId);
    }

    public int size() {
        return 1;
    }
}

