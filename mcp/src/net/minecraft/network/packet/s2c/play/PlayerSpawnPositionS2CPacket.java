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

public class PlayerSpawnPositionS2CPacket
extends Packet {
    public int x;
    public int y;
    public int z;

    public PlayerSpawnPositionS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public PlayerSpawnPositionS2CPacket(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerSpawnPosition(this);
    }

    public int size() {
        return 12;
    }
}

