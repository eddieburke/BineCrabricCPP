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

public class EntityS2CPacket
extends Packet {
    public int id;
    public byte deltaX;
    public byte deltaY;
    public byte deltaZ;
    public byte yaw;
    public byte pitch;
    public boolean rotate = false;

    public EntityS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityS2CPacket(int entityId) {
        this.id = entityId;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntity(this);
    }

    public int size() {
        return 4;
    }
}

