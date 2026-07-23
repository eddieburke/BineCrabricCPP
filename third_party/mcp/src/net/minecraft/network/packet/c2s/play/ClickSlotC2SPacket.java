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

public class ClickSlotC2SPacket
extends Packet {
    public int syncId;
    public int slot;
    public int button;
    public short actionType;
    public ItemStack stack;
    public boolean holdingShift;

    public ClickSlotC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public ClickSlotC2SPacket(int syncId, int slot, int button, boolean holdingShift, ItemStack stack, short actionType) {
        this.syncId = syncId;
        this.slot = slot;
        this.button = button;
        this.stack = stack;
        this.actionType = actionType;
        this.holdingShift = holdingShift;
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onClickSlot(this);
    }

    public void read(DataInputStream stream) {
        this.syncId = stream.readByte();
        this.slot = stream.readShort();
        this.button = stream.readByte();
        this.actionType = stream.readShort();
        this.holdingShift = stream.readBoolean();
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
        stream.writeByte(this.button);
        stream.writeShort(this.actionType);
        stream.writeBoolean(this.holdingShift);
        if (this.stack == null) {
            stream.writeShort(-1);
        } else {
            stream.writeShort(this.stack.itemId);
            stream.writeByte(this.stack.count);
            stream.writeShort(this.stack.getDamage());
        }
    }

    public int size() {
        return 11;
    }
}

