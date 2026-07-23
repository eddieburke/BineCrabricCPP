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
import java.io.IOException;
import java.util.zip.DataFormatException;
import java.util.zip.Deflater;
import java.util.zip.Inflater;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.world.World;

public class ChunkDataS2CPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public int sizeX;
    public int sizeY;
    public int sizeZ;
    public byte[] chunkData;
    private int chunkDataSize;

    public ChunkDataS2CPacket() {
        this.worldPacket = true;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Environment(value=EnvType.SERVER)
    public ChunkDataS2CPacket(int x, int y, int z, int sizeX, int sizeY, int sizeZ, World world) {
        this.worldPacket = true;
        this.x = x;
        this.y = y;
        this.z = z;
        this.sizeX = sizeX;
        this.sizeY = sizeY;
        this.sizeZ = sizeZ;
        byte[] byArray = world.getChunkData(x, y, z, sizeX, sizeY, sizeZ);
        Deflater deflater = new Deflater(-1);
        try {
            deflater.setInput(byArray);
            deflater.finish();
            this.chunkData = new byte[sizeX * sizeY * sizeZ * 5 / 2];
            this.chunkDataSize = deflater.deflate(this.chunkData);
        }
        finally {
            deflater.end();
        }
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.readShort();
        this.z = stream.readInt();
        this.sizeX = stream.read() + 1;
        this.sizeY = stream.read() + 1;
        this.sizeZ = stream.read() + 1;
        this.chunkDataSize = stream.readInt();
        byte[] byArray = new byte[this.chunkDataSize];
        stream.readFully(byArray);
        this.chunkData = new byte[this.sizeX * this.sizeY * this.sizeZ * 5 / 2];
        Inflater inflater = new Inflater();
        inflater.setInput(byArray);
        try {
            inflater.inflate(this.chunkData);
        }
        catch (DataFormatException dataFormatException) {
            throw new IOException("Bad compressed data format");
        }
        finally {
            inflater.end();
        }
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.writeShort(this.y);
        stream.writeInt(this.z);
        stream.write(this.sizeX - 1);
        stream.write(this.sizeY - 1);
        stream.write(this.sizeZ - 1);
        stream.writeInt(this.chunkDataSize);
        stream.write(this.chunkData, 0, this.chunkDataSize);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.handleChunkData(this);
    }

    public int size() {
        return 17 + this.chunkDataSize;
    }
}

