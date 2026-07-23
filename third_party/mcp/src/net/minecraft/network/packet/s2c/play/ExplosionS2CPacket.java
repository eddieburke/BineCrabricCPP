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
import java.util.HashSet;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.BlockPos;

public class ExplosionS2CPacket
extends Packet {
    public double x;
    public double y;
    public double z;
    public float radius;
    public Set affectedBlocks;

    public ExplosionS2CPacket() {
    }

    @Environment(value=EnvType.SERVER)
    public ExplosionS2CPacket(double x, double y, double z, float radius, Set affectedBlocks) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.radius = radius;
        this.affectedBlocks = new HashSet(affectedBlocks);
    }

    public void read(DataInputStream stream) {
        this.x = stream.readDouble();
        this.y = stream.readDouble();
        this.z = stream.readDouble();
        this.radius = stream.readFloat();
        int n = stream.readInt();
        this.affectedBlocks = new HashSet();
        int n2 = (int)this.x;
        int n3 = (int)this.y;
        int n4 = (int)this.z;
        for (int i = 0; i < n; ++i) {
            int n5 = stream.readByte() + n2;
            int n6 = stream.readByte() + n3;
            int n7 = stream.readByte() + n4;
            this.affectedBlocks.add(new BlockPos(n5, n6, n7));
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeDouble(this.x);
        stream.writeDouble(this.y);
        stream.writeDouble(this.z);
        stream.writeFloat(this.radius);
        stream.writeInt(this.affectedBlocks.size());
        int n = (int)this.x;
        int n2 = (int)this.y;
        int n3 = (int)this.z;
        for (BlockPos blockPos : this.affectedBlocks) {
            int n4 = blockPos.x - n;
            int n5 = blockPos.y - n2;
            int n6 = blockPos.z - n3;
            stream.writeByte(n4);
            stream.writeByte(n5);
            stream.writeByte(n6);
        }
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onExplosion(this);
    }

    public int size() {
        return 32 + this.affectedBlocks.size() * 3;
    }
}

