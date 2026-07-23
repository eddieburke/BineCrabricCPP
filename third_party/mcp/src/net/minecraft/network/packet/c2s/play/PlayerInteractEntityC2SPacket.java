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
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PlayerInteractEntityC2SPacket
extends Packet {
    public int playerId;
    public int entityId;
    public int isLeftClick;

    public PlayerInteractEntityC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerInteractEntityC2SPacket(int playerId, int entityId, int isLeftClick) {
        this.playerId = playerId;
        this.entityId = entityId;
        this.isLeftClick = isLeftClick;
    }

    public void read(DataInputStream stream) {
        this.playerId = stream.readInt();
        this.entityId = stream.readInt();
        this.isLeftClick = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.playerId);
        stream.writeInt(this.entityId);
        stream.writeByte(this.isLeftClick);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.handleInteractEntity(this);
    }

    public int size() {
        return 9;
    }
}

