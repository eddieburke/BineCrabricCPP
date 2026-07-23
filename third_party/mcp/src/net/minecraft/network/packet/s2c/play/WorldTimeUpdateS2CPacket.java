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

public class WorldTimeUpdateS2CPacket
extends Packet {
    public long time;

    public WorldTimeUpdateS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public WorldTimeUpdateS2CPacket(long time) {
        this.time = time;
    }

    public void read(DataInputStream stream) {
        this.time = stream.readLong();
    }

    public void write(DataOutputStream stream) {
        stream.writeLong(this.time);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onWorldTimeUpdate(this);
    }

    public int size() {
        return 8;
    }
}

