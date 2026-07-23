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
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.MathHelper;

public class EntityPositionS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public byte yaw;
    public byte pitch;

    public EntityPositionS2CPacket() {
    }

    public EntityPositionS2CPacket(Entity entity) {
        this.id = entity.id;
        this.x = MathHelper.floor(entity.x * 32.0);
        this.y = MathHelper.floor(entity.y * 32.0);
        this.z = MathHelper.floor(entity.z * 32.0);
        this.yaw = (byte)(entity.yaw * 256.0f / 360.0f);
        this.pitch = (byte)(entity.pitch * 256.0f / 360.0f);
    }

    @Environment(value=EnvType.SERVER)
    public EntityPositionS2CPacket(int entityId, int x, int y, int z, byte yaw, byte pitch) {
        this.id = entityId;
        this.x = x;
        this.y = y;
        this.z = z;
        this.yaw = yaw;
        this.pitch = pitch;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.yaw = (byte)stream.read();
        this.pitch = (byte)stream.read();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.write(this.yaw);
        stream.write(this.pitch);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityPosition(this);
    }

    public int size() {
        return 34;
    }
}

