/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.MathHelper;

public class PlayerSpawnS2CPacket
extends Packet {
    public int id;
    public String name;
    public int x;
    public int y;
    public int z;
    public byte yaw;
    public byte pitch;
    public int itemRawId;

    public PlayerSpawnS2CPacket() {
    }

    public PlayerSpawnS2CPacket(PlayerEntity player) {
        this.id = player.id;
        this.name = player.name;
        this.x = MathHelper.floor(player.x * 32.0);
        this.y = MathHelper.floor(player.y * 32.0);
        this.z = MathHelper.floor(player.z * 32.0);
        this.yaw = (byte)(player.yaw * 256.0f / 360.0f);
        this.pitch = (byte)(player.pitch * 256.0f / 360.0f);
        ItemStack itemStack = player.inventory.getSelectedItem();
        this.itemRawId = itemStack == null ? 0 : itemStack.itemId;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.name = PlayerSpawnS2CPacket.readString(stream, 16);
        this.x = stream.readInt();
        this.y = stream.readInt();
        this.z = stream.readInt();
        this.yaw = stream.readByte();
        this.pitch = stream.readByte();
        this.itemRawId = stream.readShort();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        PlayerSpawnS2CPacket.writeString(this.name, stream);
        stream.writeInt(this.x);
        stream.writeInt(this.y);
        stream.writeInt(this.z);
        stream.writeByte(this.yaw);
        stream.writeByte(this.pitch);
        stream.writeShort(this.itemRawId);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerSpawn(this);
    }

    public int size() {
        return 28;
    }
}

