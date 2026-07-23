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
import net.minecraft.network.packet.play.PlayerMovePacket;

public class PlayerMovePositionAndOnGroundPacket
extends PlayerMovePacket {
    public PlayerMovePositionAndOnGroundPacket() {
        this.changePosition = true;
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerMovePositionAndOnGroundPacket(double x, double y, double eyeHeight, double z, boolean onGround) {
        this.x = x;
        this.y = y;
        this.eyeHeight = eyeHeight;
        this.z = z;
        this.onGround = onGround;
        this.changePosition = true;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readDouble();
        this.y = stream.readDouble();
        this.eyeHeight = stream.readDouble();
        this.z = stream.readDouble();
        super.read(stream);
    }

    public void write(DataOutputStream stream) {
        stream.writeDouble(this.x);
        stream.writeDouble(this.y);
        stream.writeDouble(this.eyeHeight);
        stream.writeDouble(this.z);
        super.write(stream);
    }

    public int size() {
        return 33;
    }
}

