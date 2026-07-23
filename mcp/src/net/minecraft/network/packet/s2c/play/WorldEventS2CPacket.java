/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class WorldEventS2CPacket
extends Packet {
    public int eventId;
    public int data;
    public int x;
    public int y;
    public int z;

    public WorldEventS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public WorldEventS2CPacket(int eventId, int x, int y, int z, int data) {
        this.eventId = eventId;
        this.x = x;
        this.y = y;
        this.z = z;
        this.data = data;
    }

    public void read(DataInputStream stream) {
        this.eventId = stream.readInt();
        this.x = stream.readInt();
        this.y = stream.readByte();
        this.z = stream.readInt();
        this.data = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.eventId);
        stream.writeInt(this.x);
        stream.writeByte(this.y);
        stream.writeInt(this.z);
        stream.writeInt(this.data);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onWorldEvent(this);
    }

    public int size() {
        return 20;
    }
}

