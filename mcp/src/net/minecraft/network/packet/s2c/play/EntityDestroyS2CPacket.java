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
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class EntityDestroyS2CPacket
extends Packet {
    public int id;

    public EntityDestroyS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityDestroyS2CPacket(int id) {
        this.id = id;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityDestroy(this);
    }

    public int size() {
        return 4;
    }
}

