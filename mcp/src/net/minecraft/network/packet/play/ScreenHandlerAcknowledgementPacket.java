/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class ScreenHandlerAcknowledgementPacket
extends Packet {
    public int syncId;
    public short actionType;
    public boolean accepted;

    public ScreenHandlerAcknowledgementPacket() {
    }

    public ScreenHandlerAcknowledgementPacket(int syncId, short actionType, boolean accepter) {
        this.syncId = syncId;
        this.actionType = actionType;
        this.accepted = accepter;
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onScreenHandlerAcknowledgement(this);
    }

    public void read(DataInputStream stream) {
        this.syncId = stream.readByte();
        this.actionType = stream.readShort();
        this.accepted = stream.readByte() != 0;
    }

    public void write(DataOutputStream stream) {
        stream.writeByte(this.syncId);
        stream.writeShort(this.actionType);
        stream.writeByte(this.accepted ? 1 : 0);
    }

    public int size() {
        return 4;
    }
}

