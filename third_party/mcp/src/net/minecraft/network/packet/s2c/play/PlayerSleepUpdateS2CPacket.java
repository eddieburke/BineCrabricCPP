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
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PlayerSleepUpdateS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public int status;

    public PlayerSleepUpdateS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public PlayerSleepUpdateS2CPacket(Entity player, int status, int x, int y, int z) {
        this.status = status;
        this.x = x;
        this.y = y;
        this.z = z;
        this.id = player.id;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.status = stream.readByte();
        this.x = stream.readInt();
        this.y = stream.readByte();
        this.z = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.status);
        stream.writeInt(this.x);
        stream.writeByte(this.y);
        stream.writeInt(this.z);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerSleepUpdate(this);
    }

    public int size() {
        return 14;
    }
}

