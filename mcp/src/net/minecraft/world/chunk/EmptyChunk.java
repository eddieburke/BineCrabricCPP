/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.chunk;

import java.util.Arrays;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.world.LightType;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public class EmptyChunk
extends Chunk {
    public EmptyChunk(World arg, int i, int j) {
        super(arg, i, j);
        this.empty = true;
    }

    public EmptyChunk(World arg, byte[] bs, int i, int j) {
        super(arg, bs, i, j);
        this.empty = true;
    }

    public boolean chunkPosEquals(int x, int z) {
        return x == this.x && z == this.z;
    }

    public int getHeight(int x, int z) {
        return 0;
    }

    public void onLoad() {
    }

    @Environment(value=EnvType.CLIENT)
    public void populateHeightMapOnly() {
    }

    public void populateHeightMap() {
    }

    public void populateBlockLight() {
    }

    public int getBlockId(int x, int y, int z) {
        return 0;
    }

    public boolean setBlock(int x, int y, int z, int rawId, int meta) {
        return true;
    }

    public boolean setBlock(int x, int y, int z, int rawId) {
        return true;
    }

    public int getBlockMeta(int x, int y, int z) {
        return 0;
    }

    public void setBlockMeta(int x, int y, int z, int meta) {
    }

    public int getLight(LightType lightType, int x, int y, int z) {
        return 0;
    }

    public void setLight(LightType lightType, int x, int y, int z, int value) {
    }

    public int getLight(int x, int y, int z, int ambientDarkness) {
        return 0;
    }

    public void addEntity(Entity entity) {
    }

    public void removeEntity(Entity entity) {
    }

    public void removeEntity(Entity entity, int chunkSlice) {
    }

    public boolean isAboveMaxHeight(int x, int y, int z) {
        return false;
    }

    public BlockEntity getBlockEntity(int x, int y, int z) {
        return null;
    }

    public void addBlockEntity(BlockEntity blockEntity) {
    }

    public void setBlockEntity(int localX, int y, int localZ, BlockEntity blockEntity) {
    }

    public void removeBlockEntityAt(int localX, int y, int localZ) {
    }

    public void load() {
    }

    public void unload() {
    }

    public void markDirty() {
    }

    public void collectOtherEntities(Entity except, Box box, List result) {
    }

    public void collectEntitiesByClass(Class entityClass, Box box, List result) {
    }

    public boolean shouldSave(boolean saveEntities) {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int loadFromPacket(byte[] bytes, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, int offset) {
        int n = maxX - minX;
        int n2 = maxY - minY;
        int n3 = maxZ - minZ;
        int n4 = n * n2 * n3;
        return n4 + n4 / 2 * 3;
    }

    @Environment(value=EnvType.SERVER)
    public int toPacket(byte[] bytes, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, int offset) {
        int n = maxX - minX;
        int n2 = maxY - minY;
        int n3 = maxZ - minZ;
        int n4 = n * n2 * n3;
        int n5 = n4 + n4 / 2 * 3;
        Arrays.fill(bytes, offset, offset + n5, (byte)0);
        return n5;
    }

    public Random getSlimeRandom(long scrambler) {
        return new Random(this.world.getSeed() + (long)(this.x * this.x * 4987142) + (long)(this.x * 5947611) + (long)(this.z * this.z) * 4392871L + (long)(this.z * 389711) ^ scrambler);
    }

    public boolean isEmpty() {
        return true;
    }
}

