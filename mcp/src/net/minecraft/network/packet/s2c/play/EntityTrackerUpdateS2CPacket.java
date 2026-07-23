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
import net.minecraft.entity.data.DataTracker;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class EntityTrackerUpdateS2CPacket
extends Packet {
    public int id;
    private List trackedValues;

    public EntityTrackerUpdateS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public EntityTrackerUpdateS2CPacket(int entityId, DataTracker dataTracker) {
        this.id = entityId;
        this.trackedValues = dataTracker.getDirtyEntries();
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.trackedValues = DataTracker.readEntries(stream);
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        DataTracker.writeEntries(this.trackedValues, stream);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityTrackerUpdate(this);
    }

    public int size() {
        return 5;
    }

    @Environment(value=EnvType.CLIENT)
    public List getTrackedValues() {
        return this.trackedValues;
    }
}

