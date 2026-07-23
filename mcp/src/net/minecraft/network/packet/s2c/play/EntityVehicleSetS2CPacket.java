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
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class EntityVehicleSetS2CPacket
extends Packet {
    public int id;
    public int vehicleId;

    public EntityVehicleSetS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityVehicleSetS2CPacket(Entity entity, Entity vehicle) {
        this.id = entity.id;
        this.vehicleId = vehicle != null ? vehicle.id : -1;
    }

    public int size() {
        return 8;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.vehicleId = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeInt(this.vehicleId);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityVehicleSet(this);
    }
}

