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
import net.minecraft.item.ItemStack;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class InventoryS2CPacket
extends Packet {
    public int syncId;
    public ItemStack[] contents;

    public InventoryS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public InventoryS2CPacket(int syncId, List contents) {
        this.syncId = syncId;
        this.contents = new ItemStack[contents.size()];
        for (int i = 0; i < this.contents.length; ++i) {
            ItemStack itemStack = (ItemStack)contents.get(i);
            this.contents[i] = itemStack == null ? null : itemStack.copy();
        }
    }

    public void read(DataInputStream stream) {
        this.syncId = stream.readByte();
        int n = stream.readShort();
        this.contents = new ItemStack[n];
        for (int i = 0; i < n; ++i) {
            short s = stream.readShort();
            if (s < 0) continue;
            byte by = stream.readByte();
            short s2 = stream.readShort();
            this.contents[i] = new ItemStack(s, (int)by, (int)s2);
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeByte(this.syncId);
        stream.writeShort(this.contents.length);
        for (int i = 0; i < this.contents.length; ++i) {
            if (this.contents[i] == null) {
                stream.writeShort(-1);
                continue;
            }
            stream.writeShort((short)this.contents[i].itemId);
            stream.writeByte((byte)this.contents[i].count);
            stream.writeShort((short)this.contents[i].getDamage());
        }
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onInventory(this);
    }

    public int size() {
        return 3 + this.contents.length * 5;
    }
}

