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

public class EntitySpawnS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public int velocityX;
    public int velocityY;
    public int velocityZ;
    public int entityType;
    public int entityData;

    public EntitySpawnS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntitySpawnS2CPacket(Entity entity, int entityType) {
        this(entity, entityType, 0);
    }

    @Environment(value=EnvType.SERVER)
    public EntitySpawnS2CPacket(Entity entity, int entityType, int entityData) {
        this.id = entity.id;
        this.x = MathHelper.floor(entity.x * 32.0);
        this.y = MathHelper.floor(entity.y * 32.0);
        this.z = MathHelper.floor(entity.z * 32.0);
        this.entityType = entityType;
        this.entityData = entityData;
        if (entityData > 0) {
            double d = entity.velocityX;
            double d2 = entity.velocityY;
            double d3 = entity.velocityZ;
            double d4 = 3.9;
            if (d < -d4) {
                d = -d4;
            }
            if (d2 < -d4) {
                d2 = -d4;
            }
            if (d3 < -d4) {
                d3 = -d4;
            }
            if (d > d4) {
                d = d4;
            }
            if (d2 > d4) {
                d2 = d4;
            }
            if (d3 > d4) {
                d3 = d4;
            }
            this.velocityX = (int)(d * 8000.0);
            this.velocityY = (int)(d2 * 8000.0);
            this.velocityZ = (int)(d3 * 8000.0);
        }
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.entityType = stream.readByte();
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.entityData = stream.readInt();
        if (this.entityData > 0) {
            this.velocityX = stream.readShort();
            this.velocityY = stream.readShort();
            this.velocityZ = stream.readShort();
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.entityType);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.writeInt(this.entityData);
        if (this.entityData > 0) {
            stream.writeShort(this.velocityX);
            stream.writeShort(this.velocityY);
            stream.writeShort(this.velocityZ);
        }
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntitySpawn(this);
    }

    public int size() {
        return 21 + this.entityData > 0 ? 6 : 0;
    }
}

