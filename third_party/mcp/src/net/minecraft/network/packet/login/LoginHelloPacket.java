/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network.packet.login;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class LoginHelloPacket
extends Packet {
    public int protocolVersion;
    public String username;
    public long worldSeed;
    public byte dimensionId;

    public LoginHelloPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public LoginHelloPacket(String username, int protocolVersion) {
        this.username = username;
        this.protocolVersion = protocolVersion;
    }

    @Environment(value=EnvType.SERVER)
    public LoginHelloPacket(String username, int protocolVersion, long worldSeed, byte dimensionId) {
        this.username = username;
        this.protocolVersion = protocolVersion;
        this.worldSeed = worldSeed;
        this.dimensionId = dimensionId;
    }

    public void read(DataInputStream stream) {
        this.protocolVersion = stream.readInt();
        this.username = LoginHelloPacket.readString(stream, 16);
        this.worldSeed = stream.readLong();
        this.dimensionId = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.protocolVersion);
        LoginHelloPacket.writeString(this.username, stream);
        stream.writeLong(this.worldSeed);
        stream.writeByte(this.dimensionId);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onHello(this);
    }

    public int size() {
        return 4 + this.username.length() + 4 + 5;
    }
}

