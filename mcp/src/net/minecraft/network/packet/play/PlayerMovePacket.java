/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PlayerMovePacket
extends Packet {
    public double x;
    public double y;
    public double z;
    public double eyeHeight;
    public float yaw;
    public float pitch;
    public boolean onGround;
    public boolean changePosition;
    public boolean changeLook;

    public PlayerMovePacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerMovePacket(boolean onGround) {
        this.onGround = onGround;
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerMove(this);
    }

    public void read(DataInputStream stream) {
        this.onGround = stream.read() != 0;
    }

    public void write(DataOutputStream stream) {
        stream.write(this.onGround ? 1 : 0);
    }

    public int size() {
        return 1;
    }
}

