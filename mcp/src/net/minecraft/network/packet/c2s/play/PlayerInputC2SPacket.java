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

public class PlayerInputC2SPacket
extends Packet {
    private float sideways;
    private float forward;
    private boolean jumping;
    private boolean sneaking;
    private float pitch;
    private float yaw;

    public void read(DataInputStream stream) {
        this.sideways = stream.readFloat();
        this.forward = stream.readFloat();
        this.pitch = stream.readFloat();
        this.yaw = stream.readFloat();
        this.jumping = stream.readBoolean();
        this.sneaking = stream.readBoolean();
    }

    public void write(DataOutputStream stream) {
        stream.writeFloat(this.sideways);
        stream.writeFloat(this.forward);
        stream.writeFloat(this.pitch);
        stream.writeFloat(this.yaw);
        stream.writeBoolean(this.jumping);
        stream.writeBoolean(this.sneaking);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerInput(this);
    }

    public int size() {
        return 18;
    }

    @Environment(value=EnvType.SERVER)
    public float getSideways() {
        return this.sideways;
    }

    @Environment(value=EnvType.SERVER)
    public float getPitch() {
        return this.pitch;
    }

    @Environment(value=EnvType.SERVER)
    public float getForward() {
        return this.forward;
    }

    @Environment(value=EnvType.SERVER)
    public float getYaw() {
        return this.yaw;
    }

    @Environment(value=EnvType.SERVER)
    public boolean isJumping() {
        return this.jumping;
    }

    @Environment(value=EnvType.SERVER)
    public boolean isSneaking() {
        return this.sneaking;
    }
}

