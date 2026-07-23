/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet.s2c.play;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import net.minecraft.entity.Entity;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class EntityVelocityUpdateS2CPacket
extends Packet {
    public int id;
    public int velocityX;
    public int velocityY;
    public int velocityZ;

    public EntityVelocityUpdateS2CPacket() {
    }

    public EntityVelocityUpdateS2CPacket(Entity entity) {
        this(entity.id, entity.velocityX, entity.velocityY, entity.velocityZ);
    }

    public EntityVelocityUpdateS2CPacket(int id, double velocityX, double velocityY, double velocityZ) {
        this.id = id;
        double d = 3.9;
        if (velocityX < -d) {
            velocityX = -d;
        }
        if (velocityY < -d) {
            velocityY = -d;
        }
        if (velocityZ < -d) {
            velocityZ = -d;
        }
        if (velocityX > d) {
            velocityX = d;
        }
        if (velocityY > d) {
            velocityY = d;
        }
        if (velocityZ > d) {
            velocityZ = d;
        }
        this.velocityX = (int)(velocityX * 8000.0);
        this.velocityY = (int)(velocityY * 8000.0);
        this.velocityZ = (int)(velocityZ * 8000.0);
    }

    public void read(DataInputStream stream) {
        this.id = stream.readInt();
        this.velocityX = stream.readShort();
        this.velocityY = stream.readShort();
        this.velocityZ = stream.readShort();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.id);
        stream.writeShort(this.velocityX);
        stream.writeShort(this.velocityY);
        stream.writeShort(this.velocityZ);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onEntityVelocityUpdate(this);
    }

    public int size() {
        return 10;
    }
}

