/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.MathHelper;

public class GlobalEntitySpawnS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public int type;

    public GlobalEntitySpawnS2CPacket() {
    }

    public GlobalEntitySpawnS2CPacket(Entity entity) {
        this.id = entity.id;
        this.x = MathHelper.floor(entity.x * 32.0);
        this.y = MathHelper.floor(entity.y * 32.0);
        this.z = MathHelper.floor(entity.z * 32.0);
        if (entity instanceof LightningEntity) {
            this.type = 1;
        }
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.type = stream.readByte();
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.type);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onLightningEntitySpawn(this);
    }

    public int size() {
        return 17;
    }
}

