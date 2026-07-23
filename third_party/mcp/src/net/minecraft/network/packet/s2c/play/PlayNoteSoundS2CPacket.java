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

public class PlayNoteSoundS2CPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public int instrument;
    public int pitch;

    public PlayNoteSoundS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public PlayNoteSoundS2CPacket(int x, int y, int z, int instrument, int pitch) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.instrument = instrument;
        this.pitch = pitch;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.readShort();
        this.z = stream.readInt();
        this.instrument = stream.read();
        this.pitch = stream.read();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.writeShort(this.y);
        stream.writeInt(this.z);
        stream.write(this.instrument);
        stream.write(this.pitch);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayNoteSound(this);
    }

    public int size() {
        return 12;
    }
}

