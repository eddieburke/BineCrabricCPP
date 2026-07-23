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
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.EntityRegistry;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.data.DataTracker;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.MathHelper;

public class LivingEntitySpawnS2CPacket
extends Packet {
    public int id;
    public byte entityType;
    public int x;
    public int y;
    public int z;
    public byte yaw;
    public byte pitch;
    private DataTracker dataTracker;
    private List trackedValues;

    public LivingEntitySpawnS2CPacket() {
    }

    public LivingEntitySpawnS2CPacket(LivingEntity livingEntity) {
        this.id = livingEntity.id;
        this.entityType = (byte)EntityRegistry.getRawId(livingEntity);
        this.x = MathHelper.floor(livingEntity.x * 32.0);
        this.y = MathHelper.floor(livingEntity.y * 32.0);
        this.z = MathHelper.floor(livingEntity.z * 32.0);
        this.yaw = (byte)(livingEntity.yaw * 256.0f / 360.0f);
        this.pitch = (byte)(livingEntity.pitch * 256.0f / 360.0f);
        this.dataTracker = livingEntity.getDataTracker();
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.entityType = stream.readByte();
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.yaw = stream.readByte();
        this.pitch = stream.readByte();
        this.trackedValues = DataTracker.readEntries(stream);
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.entityType);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.writeByte(this.yaw);
        stream.writeByte(this.pitch);
        this.dataTracker.writeAllEntries(stream);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onLivingEntitySpawn(this);
    }

    public int size() {
        return 20;
    }

    @Environment(value=EnvType.CLIENT)
    public List getTrackedValues() {
        return this.trackedValues;
    }
}

