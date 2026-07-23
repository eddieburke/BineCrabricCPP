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
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public class ChunkDeltaUpdateS2CPacket
extends Packet {
    public int x;
    public int z;
    public short[] positions;
    public byte[] blockRawIds;
    public byte[] blockMetadata;
    public int size;

    public ChunkDeltaUpdateS2CPacket() {
        this.worldPacket = true;
    }

    @Environment(value=EnvType.SERVER)
    public ChunkDeltaUpdateS2CPacket(int x, int z, short[] positions, int size, World world) {
        this.worldPacket = true;
        this.x = x;
        this.z = z;
        this.size = size;
        this.positions = new short[size];
        this.blockRawIds = new byte[size];
        this.blockMetadata = new byte[size];
        Chunk chunk = world.getChunk(x, z);
        for (int i = 0; i < size; ++i) {
            int n = positions[i] >> 12 & 0xF;
            int n2 = positions[i] >> 8 & 0xF;
            int n3 = positions[i] & 0xFF;
            this.positions[i] = positions[i];
            this.blockRawIds[i] = (byte)chunk.getBlockId(n, n3, n2);
            this.blockMetadata[i] = (byte)chunk.getBlockMeta(n, n3, n2);
        }
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.z = stream.readInt();
        this.size = stream.readShort() & 0xFFFF;
        this.positions = new short[this.size];
        this.blockRawIds = new byte[this.size];
        this.blockMetadata = new byte[this.size];
        for (int i = 0; i < this.size; ++i) {
            this.positions[i] = stream.readShort();
        }
        stream.readFully(this.blockRawIds);
        stream.readFully(this.blockMetadata);
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.writeInt(this.z);
        stream.writeShort((short)this.size);
        for (int i = 0; i < this.size; ++i) {
            stream.writeShort(this.positions[i]);
        }
        stream.write(this.blockRawIds);
        stream.write(this.blockMetadata);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onChunkDeltaUpdate(this);
    }

    public int size() {
        return 10 + this.size * 4;
    }
}

