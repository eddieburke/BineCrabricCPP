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

public class EntityEquipmentUpdateS2CPacket
extends Packet {
    public int id;
    public int slot;
    public int itemRawId;
    public int itemDamage;

    public EntityEquipmentUpdateS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityEquipmentUpdateS2CPacket(int id, int slot, ItemStack itemStack) {
        this.id = id;
        this.slot = slot;
        if (itemStack == null) {
            this.itemRawId = -1;
            this.itemDamage = 0;
        } else {
            this.itemRawId = itemStack.itemId;
            this.itemDamage = itemStack.getDamage();
        }
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.slot = stream.readShort();
        this.itemRawId = stream.readShort();
        this.itemDamage = stream.readShort();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeShort(this.slot);
        stream.writeShort(this.itemRawId);
        stream.writeShort(this.itemDamage);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityEquipmentUpdate(this);
    }

    public int size() {
        return 8;
    }
}

