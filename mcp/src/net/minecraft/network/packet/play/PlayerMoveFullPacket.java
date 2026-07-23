/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.network.packet.play.PlayerMovePacket;

public class PlayerMoveFullPacket
extends PlayerMovePacket {
    public PlayerMoveFullPacket() {
        this.changeLook = true;
        this.changePosition = true;
    }

    public PlayerMoveFullPacket(double x, double y, double eyeHeight, double z, float yaw, float pitch, boolean onGround) {
        this.x = x;
        this.y = y;
        this.eyeHeight = eyeHeight;
        this.z = z;
        this.yaw = yaw;
        this.pitch = pitch;
        this.onGround = onGround;
        this.changeLook = true;
        this.changePosition = true;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readDouble();
        this.y = stream.readDouble();
        this.eyeHeight = stream.readDouble();
        this.z = stream.readDouble();
        this.yaw = stream.readFloat();
        this.pitch = stream.readFloat();
        super.read(stream);
    }

    public void write(DataOutputStream stream) {
        stream.writeDouble(this.x);
        stream.writeDouble(this.y);
        stream.writeDouble(this.eyeHeight);
        stream.writeDouble(this.z);
        stream.writeFloat(this.yaw);
        stream.writeFloat(this.pitch);
        super.write(stream);
    }

    public int size() {
        return 41;
    }
}

