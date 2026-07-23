/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.decoration.painting.PaintingVariants;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PaintingEntitySpawnS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public int facing;
    public String variant;

    public PaintingEntitySpawnS2CPacket() {
    }

    public PaintingEntitySpawnS2CPacket(PaintingEntity paintingEntity) {
        this.id = paintingEntity.id;
        this.x = paintingEntity.attachmentX;
        this.y = paintingEntity.attachmentY;
        this.z = paintingEntity.attachmentZ;
        this.facing = paintingEntity.facing;
        this.variant = paintingEntity.variant.id;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.variant = PaintingEntitySpawnS2CPacket.readString(stream, PaintingVariants.LONGEST_NAME_LENGTH);
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.facing = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        PaintingEntitySpawnS2CPacket.writeString(this.variant, stream);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.writeInt(this.facing);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPaintingEntitySpawn(this);
    }

    public int size() {
        return 24;
    }
}

