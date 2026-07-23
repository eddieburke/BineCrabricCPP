/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.ItemEntity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.MathHelper;

public class ItemEntitySpawnS2CPacket
extends Packet {
    public int id;
    public int x;
    public int y;
    public int z;
    public byte velocityX;
    public byte velocityY;
    public byte velocityZ;
    public int itemRawId;
    public int itemCount;
    public int itemDamage;

    public ItemEntitySpawnS2CPacket() {
    }

    public ItemEntitySpawnS2CPacket(ItemEntity itemEntity) {
        this.id = itemEntity.id;
        this.itemRawId = itemEntity.stack.itemId;
        this.itemCount = itemEntity.stack.count;
        this.itemDamage = itemEntity.stack.getDamage();
        this.x = MathHelper.floor(itemEntity.x * 32.0);
        this.y = MathHelper.floor(itemEntity.y * 32.0);
        this.z = MathHelper.floor(itemEntity.z * 32.0);
        this.velocityX = (byte)(itemEntity.velocityX * 128.0);
        this.velocityY = (byte)(itemEntity.velocityY * 128.0);
        this.velocityZ = (byte)(itemEntity.velocityZ * 128.0);
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.itemRawId = stream.readShort();
        this.itemCount = stream.readByte();
        this.itemDamage = stream.readShort();
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.velocityX = stream.readByte();
        this.velocityY = stream.readByte();
        this.velocityZ = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeShort(this.itemRawId);
        stream.writeByte(this.itemCount);
        stream.writeShort(this.itemDamage);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.writeByte(this.velocityX);
        stream.writeByte(this.velocityY);
        stream.writeByte(this.velocityZ);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onItemEntitySpawn(this);
    }

    public int size() {
        return 24;
    }
}

