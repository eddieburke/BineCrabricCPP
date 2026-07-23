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

public class GameStateChangeS2CPacket
extends Packet {
    public static final String[] REASONS = new String[]{"tile.bed.notValid", null, null};
    public int reason;

    public GameStateChangeS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public GameStateChangeS2CPacket(int reason) {
        this.reason = reason;
    }

    public void read(DataInputStream stream) {
        this.reason = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeByte(this.reason);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onGameStateChange(this);
    }

    public int size() {
        return 1;
    }
}

