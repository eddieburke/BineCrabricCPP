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

public class MapUpdateS2CPacket
extends Packet {
    public short itemRawId;
    public short id;
    public byte[] updateData;

    public MapUpdateS2CPacket() {
        this.worldPacket = true;
    }

    @Environment(value=EnvType.SERVER)
    public MapUpdateS2CPacket(short itemRawId, short id, byte[] updateData) {
        this.worldPacket = true;
        this.itemRawId = itemRawId;
        this.id = id;
        this.updateData = updateData;
    }

    public void read(DataInputStream stream) {
        this.itemRawId = stream.readShort();
        this.id = stream.readShort();
        this.updateData = new byte[stream.readByte() & 0xFF];
        stream.readFully(this.updateData);
    }

    public void write(DataOutputStream stream) {
        stream.writeShort(this.itemRawId);
        stream.writeShort(this.id);
        stream.writeByte(this.updateData.length);
        stream.write(this.updateData);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onMapUpdate(this);
    }

    public int size() {
        return 4 + this.updateData.length;
    }
}

