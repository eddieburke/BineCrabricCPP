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

public class ItemPickupAnimationS2CPacket
extends Packet {
    public int entityId;
    public int collectorEntityId;

    public ItemPickupAnimationS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public ItemPickupAnimationS2CPacket(int entityId, int collectorId) {
        this.entityId = entityId;
        this.collectorEntityId = collectorId;
    }

    public void read(DataInputStream stream) {
        this.entityId = stream.readInt();
        this.collectorEntityId = stream.readInt();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.entityId);
        stream.writeInt(this.collectorEntityId);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onItemPickupAnimation(this);
    }

    public int size() {
        return 8;
    }
}

