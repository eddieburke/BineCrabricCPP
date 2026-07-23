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
import net.minecraft.item.ItemStack;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class ScreenHandlerSlotUpdateS2CPacket
extends Packet {
    public int syncId;
    public int slot;
    public ItemStack stack;

    public ScreenHandlerSlotUpdateS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public ScreenHandlerSlotUpdateS2CPacket(int syncId, int slot, ItemStack stack) {
        this.syncId = syncId;
        this.slot = slot;
        this.stack = stack == null ? stack : stack.copy();
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onScreenHandlerSlotUpdate(this);
    }

    public void read(DataInputStream stream) {
        this.syncId = stream.readByte();
        this.slot = stream.readShort();
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
        stream.writeByte(this.syncId);
        stream.writeShort(this.slot);
        if (this.stack == null) {
            stream.writeShort(-1);
        } else {
            stream.writeShort(this.stack.itemId);
            stream.writeByte(this.stack.count);
            stream.writeShort(this.stack.getDamage());
        }
    }

    public int size() {
        return 8;
    }
}

