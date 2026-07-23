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

public class BlockUpdateS2CPacket
extends Packet {
    public int x;
    public int y;
    public int z;
    public int blockRawId;
    public int blockMetadata;

    public BlockUpdateS2CPacket() {
        this.worldPacket = true;
    }

    @Environment(value=EnvType.SERVER)
    public BlockUpdateS2CPacket(int x, int y, int z, World world) {
        this.worldPacket = true;
        this.x = x;
        this.y = y;
        this.z = z;
        this.blockRawId = world.getBlockId(x, y, z);
        this.blockMetadata = world.getBlockMeta(x, y, z);
    }

    public void read(DataInputStream stream) {
        this.x = stream.readInt();
        this.y = stream.read();
        this.z = stream.readInt();
        this.blockRawId = stream.read();
        this.blockMetadata = stream.read();
    }

    public void write(DataOutputStream stream) {
        stream.writeInt(this.x);
        stream.write(this.y);
        stream.writeInt(this.z);
        stream.write(this.blockRawId);
        stream.write(this.blockMetadata);
    }

    public void apply(NetworkHandler networkHandler) {
        networkHandler.onBlockUpdate(this);
    }

    public int size() {
        return 11;
    }
}

