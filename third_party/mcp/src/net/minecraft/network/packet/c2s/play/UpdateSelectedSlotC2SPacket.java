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
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class UpdateSelectedSlotC2SPacket
extends Packet {
    public int selectedSlot;

    public UpdateSelectedSlotC2SPacket() {
    }

    @Environment(value=EnvType.CLIENT)
    public UpdateSelectedSlotC2SPacket(int selectedSlot) {
        this.selectedSlot = selectedSlot;
    }

    public void read(DataInputStream stream) {
        this.selectedSlot = stream.readShort();
    }

    public void write(DataOutputStream stream) {
        stream.writeShort(this.selectedSlot);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onUpdateSelectedSlot(this);
    }

    public int size() {
        return 2;
    }
}

