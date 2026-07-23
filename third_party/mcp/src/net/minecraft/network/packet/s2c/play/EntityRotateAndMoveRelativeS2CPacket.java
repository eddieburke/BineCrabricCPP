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
import net.minecraft.network.packet.s2c.play.EntityS2CPacket;

public class EntityRotateAndMoveRelativeS2CPacket
extends EntityS2CPacket {
    public EntityRotateAndMoveRelativeS2CPacket() {
        this.rotate = true;
    }

    @Environment(value=EnvType.SERVER)
    public EntityRotateAndMoveRelativeS2CPacket(int entityId, byte deltaX, byte deltaY, byte deltaZ, byte yaw, byte pitch) {
        super(entityId);
        this.deltaX = deltaX;
        this.deltaY = deltaY;
        this.deltaZ = deltaZ;
        this.yaw = yaw;
        this.pitch = pitch;
        this.rotate = true;
    }

    public void read(DataInputStream stream) {
        super.read(stream);
        this.deltaX = stream.readByte();
        this.deltaY = stream.readByte();
        this.deltaZ = stream.readByte();
        this.yaw = stream.readByte();
        this.pitch = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        super.write(stream);
        stream.writeByte(this.deltaX);
        stream.writeByte(this.deltaY);
        stream.writeByte(this.deltaZ);
        stream.writeByte(this.yaw);
        stream.writeByte(this.pitch);
    }

    public int size() {
        return 9;
    }
}

