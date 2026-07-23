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

public class EntityStatusS2CPacket
extends Packet {
    public int id;
    public byte status;

    public EntityStatusS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityStatusS2CPacket(int entityId, byte status) {
        this.id = entityId;
        this.status = status;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.status = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.status);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityStatus(this);
    }

    public int size() {
        return 5;
    }
}

