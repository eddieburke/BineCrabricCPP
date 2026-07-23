/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class EntityAnimationPacket
extends Packet {
    public int id;
    public int animationId;

    public EntityAnimationPacket() {
    }

    public EntityAnimationPacket(Entity entity, int animationId) {
        this.id = entity.id;
        this.animationId = animationId;
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.animationId = stream.readByte();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeByte(this.animationId);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityAnimation(this);
    }

    public int size() {
        return 5;
    }
}

