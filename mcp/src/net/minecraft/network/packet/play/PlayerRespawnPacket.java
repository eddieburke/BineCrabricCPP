/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PlayerRespawnPacket
extends Packet {
    public byte dimensionRawId;

    public PlayerRespawnPacket() {
    }

    public PlayerRespawnPacket(byte dimension) {
        this.dimensionRawId = dimension;
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerRespawn(this);
    }

    public void read(DataInputStream stream) {
        this.dimensionRawId = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeByte(this.dimensionRawId);
    }

    public int size() {
        return 1;
    }
}

