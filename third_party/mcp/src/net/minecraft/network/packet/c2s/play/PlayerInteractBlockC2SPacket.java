/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network.packet.c2s.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.item.ItemStack;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class PlayerInteractBlockC2SPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public int side;
    public ItemStack stack;

    public PlayerInteractBlockC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public PlayerInteractBlockC2SPacket(int x, int y, int z, int side, ItemStack stack) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.side = side;
        this.stack = stack;
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.read();
        this.z = stream.readInt();
        this.side = stream.read();
        short s = stream.readShort();
        if (s >= 0) {
            byte by = stream.readByte();
            short s2 = stream.readShort();
            this.stack = new ItemStack(s, (int)by, (int)s2);
        } else {
            this.stack = null;
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.write(this.y);
        stream.writeInt(this.z);
        stream.write(this.side);
        if (this.stack == null) {
            stream.writeShort(-1);
        } else {
            stream.writeShort(this.stack.itemId);
            stream.writeByte(this.stack.count);
            stream.writeShort(this.stack.getDamage());
        }
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onPlayerInteractBlock(this);
    }

    public int size() {
        return 15;
    }
}

