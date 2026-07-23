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

public class EntityMoveRelativeS2CPacket
extends EntityS2CPacket {
    public EntityMoveRelativeS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityMoveRelativeS2CPacket(int entityId, byte deltaX, byte deltaY, byte deltaZ) {
        super(entityId);
        this.deltaX = deltaX;
        this.deltaY = deltaY;
        this.deltaZ = deltaZ;
    }

    public void read(DataInputStream stream) {
        super.read(stream);
        this.deltaX = stream.readByte();
        this.deltaY = stream.readByte();
        this.deltaZ = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        super.write(stream);
        stream.writeByte(this.deltaX);
        stream.writeByte(this.deltaY);
        stream.writeByte(this.deltaZ);
    }

    public int size() {
        return 7;
    }
}

