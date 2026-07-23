/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network.packet.c2s.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class ClientCommandC2SPacket
extends Packet {
    public int entityId;
    public int mode;

    public ClientCommandC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public ClientCommandC2SPacket(Entity entity, int mode) {
        this.entityId = entity.id;
        this.mode = mode;
    }

    public void read(DataInputStream stream) {
        this.entityId = stream.readInt();
        this.mode = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.entityId);
        stream.writeByte(this.mode);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.handleClientCommand(this);
    }

    public int size() {
        return 5;
    }
}

