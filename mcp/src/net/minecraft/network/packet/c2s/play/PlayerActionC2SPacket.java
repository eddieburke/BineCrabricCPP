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

public class PlayerActionC2SPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public int direction;
    public int action;

    public PlayerActionC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerActionC2SPacket(int action, int x, int y, int z, int direction) {
        this.action = action;
        this.x = x;
        this.y = y;
        this.z = z;
        this.direction = direction;
    }

    public void read(DataInputStream stream) {
        this.action = stream.read();
        this.x = stream.readInt();
        this.y = stream.read();
        this.z = stream.readInt();
        this.direction = stream.read();
    }

    public void write(DataOutputStream stream) {
        stream.write(this.action);
        stream.writeInt(this.x);
        stream.write(this.y);
        stream.writeInt(this.z);
        stream.write(this.direction);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.handlePlayerAction(this);
    }

    public int size() {
        return 11;
    }
}

