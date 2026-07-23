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

public class EntityRotateS2CPacket
extends EntityS2CPacket {
    public EntityRotateS2CPacket() {
        this.rotate = true;
    }

    @Environment(value=EnvType.SERVER)
    public EntityRotateS2CPacket(int entityId, byte yaw, byte pitch) {
        super(entityId);
        this.yaw = yaw;
        this.pitch = pitch;
        this.rotate = true;
    }

    public void read(DataInputStream stream) {
        super.read(stream);
        this.yaw = stream.readByte();
        this.pitch = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        super.write(stream);
        stream.writeByte(this.yaw);
        stream.writeByte(this.pitch);
    }

    public int size() {
        return 6;
    }
}

