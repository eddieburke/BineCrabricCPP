/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class UpdateSignPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public String[] text;

    public UpdateSignPacket() {
        this.worldPacket = true;
    }

    public UpdateSignPacket(int x, int y, int z, String[] text) {
        this.worldPacket = true;
        this.x = x;
        this.y = y;
        this.z = z;
        this.text = text;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.readShort();
        this.z = stream.readInt();
        this.text = new String[4];
        for (int i = 0; i < 4; ++i) {
            this.text[i] = UpdateSignPacket.readString(stream, 15);
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.writeShort(this.y);
        stream.writeInt(this.z);
        for (int i = 0; i < 4; ++i) {
            UpdateSignPacket.writeString(this.text[i], stream);
        }
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.handleUpdateSign(this);
    }

    public int size() {
        int n = 0;
        for (int i = 0; i < 4; ++i) {
            n += this.text[i].length();
        }
        return n;
    }
}

