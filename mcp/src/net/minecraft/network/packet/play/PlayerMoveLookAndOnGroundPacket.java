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

public class PlayerMoveLookAndOnGroundPacket
extends PlayerMovePacket {
    public PlayerMoveLookAndOnGroundPacket() {
        this.changeLook = true;
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerMoveLookAndOnGroundPacket(float yaw, float pitch, boolean onGround) {
        this.yaw = yaw;
        this.pitch = pitch;
        this.onGround = onGround;
        this.changeLook = true;
    }

    public void read(DataInputStream stream) {
        this.yaw = stream.readFloat();
        this.pitch = stream.readFloat();
        super.read(stream);
    }

    public void write(DataOutputStream stream) {
        stream.writeFloat(this.yaw);
        stream.writeFloat(this.pitch);
        super.write(stream);
    }

    public int size() {
        return 9;
    }
}

