/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.chunk;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.LightType;
import net.minecraft.world.World;
import net.minecraft.world.chunk.BlockSource;
import net.minecraft.world.chunk.ChunkNibbleArray;

public class Chunk {
    public static boolean hasSkyLight;
    public byte[] blocks;
    public boolean loaded;
    public World world;
    public ChunkNibbleArray meta;
    public ChunkNibbleArray skyLight;
    public ChunkNibbleArray blockLight;
    public byte[] heightmap;
    public int minHeightmapValue;
    public final int x;
    public final int z;
    public Map blockEntities = new HashMap();
    public List[] entities = new List[8];
    public boolean terrainPopulated = false;
    public boolean dirty = false;
    public boolean empty;
    public boolean lastSaveHadEntities = false;
    public long lastSaveTime = 0L;

    public Chunk(World world, int x, int z) {
        this.world = world;
        this.x = x;
        this.z = z;
        this.heightmap = new byte[256];
        for (int i = 0; i < this.entities.length; ++i) {
            this.entities[i] = new ArrayList();
        }
    }

    public Chunk(World world, byte[] blocks, int x, int z) {
        this(world, x, z);
        this.blocks = blocks;
        this.meta = new ChunkNibbleArray(blocks.length);
        this.skyLight = new ChunkNibbleArray(blocks.length);
        this.blockLight = new ChunkNibbleArray(blocks.length);
    }

    public boolean chunkPosEquals(int x, int z) {
        return x == this.x && z == this.z;
    }

    public int getHeight(int x, int z) {
        return this.heightmap[z << 4 | x] & 0xFF;
    }

    public void onLoad() {
    }

    @Environment(value=EnvType.CLIENT)
    public void populateHeightMapOnly() {
        int n = 127;
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                int n2;
                int n3 = i << 11 | j << 7;
                for (n2 = 127; n2 > 0 && Block.BLOCKS_LIGHT_OPACITY[this.blocks[n3 + n2 - 1] & 0xFF] == 0; --n2) {
                }
                this.heightmap[j << 4 | i] = (byte)n2;
                if (n2 >= n) continue;
                n = n2;
            }
        }
        this.minHeightmapValue = n;
        this.dirty = true;
    }

    public void populateHeightMap() {
        int n;
        int n2;
        int n3 = 127;
        for (n2 = 0; n2 < 16; ++n2) {
            for (n = 0; n < 16; ++n) {
                int n4;
                int n5 = n2 << 11 | n << 7;
                for (n4 = 127; n4 > 0 && Block.BLOCKS_LIGHT_OPACITY[this.blocks[n5 + n4 - 1] & 0xFF] == 0; --n4) {
                }
                this.heightmap[n << 4 | n2] = (byte)n4;
                if (n4 < n3) {
                    n3 = n4;
                }
                if (this.world.dimension.hasCeiling) continue;
                int n6 = 15;
                int n7 = 127;
                do {
                    if ((n6 -= Block.BLOCKS_LIGHT_OPACITY[this.blocks[n5 + n7] & 0xFF]) <= 0) continue;
                    this.skyLight.set(n2, n7, n, n6);
                } while (--n7 > 0 && n6 > 0);
            }
        }
        this.minHeightmapValue = n3;
        for (n2 = 0; n2 < 16; ++n2) {
            for (n = 0; n < 16; ++n) {
                this.lightGaps(n2, n);
            }
        }
        this.dirty = true;
    }

    public void populateBlockLight() {
    }

    private void lightGaps(int x, int z) {
        int n = this.getHeight(x, z);
        int n2 = this.x * 16 + x;
        int n3 = this.z * 16 + z;
        this.lightGap(n2 - 1, n3, n);
        this.lightGap(n2 + 1, n3, n);
        this.lightGap(n2, n3 - 1, n);
        this.lightGap(n2, n3 + 1, n);
    }

    private void lightGap(int x, int z, int y) {
        int n = this.world.getTopY(x, z);
        if (n > y) {
            this.world.queueLightUpdate(LightType.SKY, x, y, z, x, n, z);
            this.dirty = true;
        } else if (n < y) {
            this.world.queueLightUpdate(LightType.SKY, x, n, z, x, y, z);
            this.dirty = true;
        }
    }

    private void updateHeightMap(int localX, int y, int localZ) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5 = n4 = this.heightmap[localZ << 4 | localX] & 0xFF;
        if (y > n4) {
            n5 = y;
        }
        int n6 = localX << 11 | localZ << 7;
        while (n5 > 0 && Block.BLOCKS_LIGHT_OPACITY[this.blocks[n6 + n5 - 1] & 0xFF] == 0) {
            --n5;
        }
        if (n5 == n4) {
            return;
        }
        this.world.setBlocksDirty(localX, localZ, n5, n4);
        this.heightmap[localZ << 4 | localX] = (byte)n5;
        if (n5 < this.minHeightmapValue) {
            this.minHeightmapValue = n5;
        } else {
            n3 = 127;
            for (n2 = 0; n2 < 16; ++n2) {
                for (n = 0; n < 16; ++n) {
                    if ((this.heightmap[n << 4 | n2] & 0xFF) >= n3) continue;
                    n3 = this.heightmap[n << 4 | n2] & 0xFF;
                }
            }
            this.minHeightmapValue = n3;
        }
        n3 = this.x * 16 + localX;
        n2 = this.z * 16 + localZ;
        if (n5 < n4) {
            for (n = n5; n < n4; ++n) {
                this.skyLight.set(localX, n, localZ, 15);
            }
        } else {
            this.world.queueLightUpdate(LightType.SKY, n3, n4, n2, n3, n5, n2);
            for (n = n4; n < n5; ++n) {
                this.skyLight.set(localX, n, localZ, 0);
            }
        }
        n = 15;
        int n7 = n5;
        while (n5 > 0 && n > 0) {
            int n8;
            if ((n8 = Block.BLOCKS_LIGHT_OPACITY[this.getBlockId(localX, --n5, localZ)]) == 0) {
                n8 = 1;
            }
            if ((n -= n8) < 0) {
                n = 0;
            }
            this.skyLight.set(localX, n5, localZ, n);
        }
        while (n5 > 0 && Block.BLOCKS_LIGHT_OPACITY[this.getBlockId(localX, n5 - 1, localZ)] == 0) {
            --n5;
        }
        if (n5 != n7) {
            this.world.queueLightUpdate(LightType.SKY, n3 - 1, n5, n2 - 1, n3 + 1, n7, n2 + 1);
        }
        this.dirty = true;
    }

    public int getBlockId(int x, int y, int z) {
        return this.blocks[x << 11 | z << 7 | y] & 0xFF;
    }

    public boolean setBlock(int x, int y, int z, int rawId, int meta) {
        byte by = (byte)rawId;
        int n = this.heightmap[z << 4 | x] & 0xFF;
        int n2 = this.blocks[x << 11 | z << 7 | y] & 0xFF;
        if (n2 == rawId && this.meta.get(x, y, z) == meta) {
            return false;
        }
        int n3 = this.x * 16 + x;
        int n4 = this.z * 16 + z;
        this.blocks[x << 11 | z << 7 | y] = (byte)(by & 0xFF);
        if (n2 != 0 && !this.world.isRemote) {
            Block.BLOCKS[n2].onBreak(this.world, n3, y, n4);
        }
        this.meta.set(x, y, z, meta);
        if (!this.world.dimension.hasCeiling) {
            if (Block.BLOCKS_LIGHT_OPACITY[by & 0xFF] != 0) {
                if (y >= n) {
                    this.updateHeightMap(x, y + 1, z);
                }
            } else if (y == n - 1) {
                this.updateHeightMap(x, y, z);
            }
            this.world.queueLightUpdate(LightType.SKY, n3, y, n4, n3, y, n4);
        }
        this.world.queueLightUpdate(LightType.BLOCK, n3, y, n4, n3, y, n4);
        this.lightGaps(x, z);
        this.meta.set(x, y, z, meta);
        if (rawId != 0) {
            Block.BLOCKS[rawId].onPlaced(this.world, n3, y, n4);
        }
        this.dirty = true;
        return true;
    }

    public boolean setBlock(int x, int y, int z, int rawId) {
        byte by = (byte)rawId;
        int n = this.heightmap[z << 4 | x] & 0xFF;
        int n2 = this.blocks[x << 11 | z << 7 | y] & 0xFF;
        if (n2 == rawId) {
            return false;
        }
        int n3 = this.x * 16 + x;
        int n4 = this.z * 16 + z;
        this.blocks[x << 11 | z << 7 | y] = (byte)(by & 0xFF);
        if (n2 != 0) {
            Block.BLOCKS[n2].onBreak(this.world, n3, y, n4);
        }
        this.meta.set(x, y, z, 0);
        if (Block.BLOCKS_LIGHT_OPACITY[by & 0xFF] != 0) {
            if (y >= n) {
                this.updateHeightMap(x, y + 1, z);
            }
        } else if (y == n - 1) {
            this.updateHeightMap(x, y, z);
        }
        this.world.queueLightUpdate(LightType.SKY, n3, y, n4, n3, y, n4);
        this.world.queueLightUpdate(LightType.BLOCK, n3, y, n4, n3, y, n4);
        this.lightGaps(x, z);
        if (rawId != 0 && !this.world.isRemote) {
            Block.BLOCKS[rawId].onPlaced(this.world, n3, y, n4);
        }
        this.dirty = true;
        return true;
    }

    public int getBlockMeta(int x, int y, int z) {
        return this.meta.get(x, y, z);
    }

    public void setBlockMeta(int x, int y, int z, int meta) {
        this.dirty = true;
        this.meta.set(x, y, z, meta);
    }

    public int getLight(LightType lightType, int x, int y, int z) {
        if (lightType == LightType.SKY) {
            return this.skyLight.get(x, y, z);
        }
        if (lightType == LightType.BLOCK) {
            return this.blockLight.get(x, y, z);
        }
        return 0;
    }

    public void setLight(LightType lightType, int x, int y, int z, int value) {
        this.dirty = true;
        if (lightType == LightType.SKY) {
            this.skyLight.set(x, y, z, value);
        } else if (lightType == LightType.BLOCK) {
            this.blockLight.set(x, y, z, value);
        } else {
            return;
        }
    }

    public int getLight(int x, int y, int z, int ambientDarkness) {
        int n;
        int n2 = this.skyLight.get(x, y, z);
        if (n2 > 0) {
            hasSkyLight = true;
        }
        if ((n = this.blockLight.get(x, y, z)) > (n2 -= ambientDarkness)) {
            n2 = n;
        }
        return n2;
    }

    public void addEntity(Entity entity) {
        int n;
        this.lastSaveHadEntities = true;
        int n2 = MathHelper.floor(entity.x / 16.0);
        int n3 = MathHelper.floor(entity.z / 16.0);
        if (n2 != this.x || n3 != this.z) {
            System.out.println("Wrong location! " + entity);
            Thread.dumpStack();
        }
        if ((n = MathHelper.floor(entity.y / 16.0)) < 0) {
            n = 0;
        }
        if (n >= this.entities.length) {
            n = this.entities.length - 1;
        }
        entity.isPersistent = true;
        entity.chunkX = this.x;
        entity.chunkSlice = n;
        entity.chunkZ = this.z;
        this.entities[n].add(entity);
    }

    public void removeEntity(Entity entity) {
        this.removeEntity(entity, entity.chunkSlice);
    }

    public void removeEntity(Entity entity, int chunkSlice) {
        if (chunkSlice < 0) {
            chunkSlice = 0;
        }
        if (chunkSlice >= this.entities.length) {
            chunkSlice = this.entities.length - 1;
        }
        this.entities[chunkSlice].remove(entity);
    }

    public boolean isAboveMaxHeight(int x, int y, int z) {
        return y >= (this.heightmap[z << 4 | x] & 0xFF);
    }

    public BlockEntity getBlockEntity(int x, int y, int z) {
        BlockPos blockPos = new BlockPos(x, y, z);
        BlockEntity blockEntity = (BlockEntity)this.blockEntities.get(blockPos);
        if (blockEntity == null) {
            int n = this.getBlockId(x, y, z);
            if (!Block.BLOCKS_WITH_ENTITY[n]) {
                return null;
            }
            BlockWithEntity blockWithEntity = (BlockWithEntity)Block.BLOCKS[n];
            blockWithEntity.onPlaced(this.world, this.x * 16 + x, y, this.z * 16 + z);
            blockEntity = (BlockEntity)this.blockEntities.get(blockPos);
        }
        if (blockEntity != null && blockEntity.isRemoved()) {
            this.blockEntities.remove(blockPos);
            return null;
        }
        return blockEntity;
    }

    public void addBlockEntity(BlockEntity blockEntity) {
        int n = blockEntity.x - this.x * 16;
        int n2 = blockEntity.y;
        int n3 = blockEntity.z - this.z * 16;
        this.setBlockEntity(n, n2, n3, blockEntity);
        if (this.loaded) {
            this.world.blockEntities.add(blockEntity);
        }
    }

    public void setBlockEntity(int localX, int y, int localZ, BlockEntity blockEntity) {
        BlockPos blockPos = new BlockPos(localX, y, localZ);
        blockEntity.world = this.world;
        blockEntity.x = this.x * 16 + localX;
        blockEntity.y = y;
        blockEntity.z = this.z * 16 + localZ;
        if (this.getBlockId(localX, y, localZ) == 0 || !(Block.BLOCKS[this.getBlockId(localX, y, localZ)] instanceof BlockWithEntity)) {
            System.out.println("Attempted to place a tile entity where there was no entity tile!");
            return;
        }
        blockEntity.cancelRemoval();
        this.blockEntities.put(blockPos, blockEntity);
    }

    public void removeBlockEntityAt(int localX, int y, int localZ) {
        BlockEntity blockEntity;
        BlockPos blockPos = new BlockPos(localX, y, localZ);
        if (this.loaded && (blockEntity = (BlockEntity)this.blockEntities.remove(blockPos)) != null) {
            blockEntity.markRemoved();
        }
    }

    public void load() {
        this.loaded = true;
        this.world.processBlockUpdates(this.blockEntities.values());
        for (int i = 0; i < this.entities.length; ++i) {
            this.world.addEntities(this.entities[i]);
        }
    }

    public void unload() {
        this.loaded = false;
        for (BlockEntity blockEntity : this.blockEntities.values()) {
            blockEntity.markRemoved();
        }
        for (int i = 0; i < this.entities.length; ++i) {
            this.world.unloadEntities(this.entities[i]);
        }
    }

    public void markDirty() {
        this.dirty = true;
    }

    public void collectOtherEntities(Entity except, Box box, List result) {
        int n = MathHelper.floor((box.minY - 2.0) / 16.0);
        int n2 = MathHelper.floor((box.maxY + 2.0) / 16.0);
        if (n < 0) {
            n = 0;
        }
        if (n2 >= this.entities.length) {
            n2 = this.entities.length - 1;
        }
        for (int i = n; i <= n2; ++i) {
            List list = this.entities[i];
            for (int j = 0; j < list.size(); ++j) {
                Entity entity = (Entity)list.get(j);
                if (entity == except || !entity.boundingBox.intersects(box)) continue;
                result.add(entity);
            }
        }
    }

    public void collectEntitiesByClass(Class entityClass, Box box, List result) {
        int n = MathHelper.floor((box.minY - 2.0) / 16.0);
        int n2 = MathHelper.floor((box.maxY + 2.0) / 16.0);
        if (n < 0) {
            n = 0;
        }
        if (n2 >= this.entities.length) {
            n2 = this.entities.length - 1;
        }
        for (int i = n; i <= n2; ++i) {
            List list = this.entities[i];
            for (int j = 0; j < list.size(); ++j) {
                Entity entity = (Entity)list.get(j);
                if (!entityClass.isAssignableFrom(entity.getClass()) || !entity.boundingBox.intersects(box)) continue;
                result.add(entity);
            }
        }
    }

    public boolean shouldSave(boolean saveEntities) {
        if (this.empty) {
            return false;
        }
        if (saveEntities ? this.lastSaveHadEntities && this.world.getTime() != this.lastSaveTime : this.lastSaveHadEntities && this.world.getTime() >= this.lastSaveTime + 600L) {
            return true;
        }
        return this.dirty;
    }

    @Environment(value=EnvType.CLIENT)
    public int loadFromPacket(byte[] bytes, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, int offset) {
        int n;
        int n2;
        int n3;
        int n4;
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = n4 << 11 | n3 << 7 | minY;
                n = maxY - minY;
                System.arraycopy(bytes, offset, this.blocks, n2, n);
                offset += n;
            }
        }
        this.populateHeightMapOnly();
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(bytes, offset, this.meta.bytes, n2, n);
                offset += n;
            }
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(bytes, offset, this.blockLight.bytes, n2, n);
                offset += n;
            }
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(bytes, offset, this.skyLight.bytes, n2, n);
                offset += n;
            }
        }
        return offset;
    }

    @Environment(value=EnvType.SERVER)
    public int toPacket(byte[] bytes, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, int offset) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5 = maxX - minX;
        int n6 = maxY - minY;
        int n7 = maxZ - minZ;
        if (n5 * n6 * n7 == this.blocks.length) {
            System.arraycopy(this.blocks, 0, bytes, offset, this.blocks.length);
            System.arraycopy(this.meta.bytes, 0, bytes, offset += this.blocks.length, this.meta.bytes.length);
            System.arraycopy(this.blockLight.bytes, 0, bytes, offset += this.meta.bytes.length, this.blockLight.bytes.length);
            System.arraycopy(this.skyLight.bytes, 0, bytes, offset += this.blockLight.bytes.length, this.skyLight.bytes.length);
            return offset += this.skyLight.bytes.length;
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = n4 << 11 | n3 << 7 | minY;
                n = maxY - minY;
                System.arraycopy(this.blocks, n2, bytes, offset, n);
                offset += n;
            }
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(this.meta.bytes, n2, bytes, offset, n);
                offset += n;
            }
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(this.blockLight.bytes, n2, bytes, offset, n);
                offset += n;
            }
        }
        for (n4 = minX; n4 < maxX; ++n4) {
            for (n3 = minZ; n3 < maxZ; ++n3) {
                n2 = (n4 << 11 | n3 << 7 | minY) >> 1;
                n = (maxY - minY) / 2;
                System.arraycopy(this.skyLight.bytes, n2, bytes, offset, n);
                offset += n;
            }
        }
        return offset;
    }

    public Random getSlimeRandom(long scrambler) {
        return new Random(this.world.getSeed() + (long)(this.x * this.x * 4987142) + (long)(this.x * 5947611) + (long)(this.z * this.z) * 4392871L + (long)(this.z * 389711) ^ scrambler);
    }

    public boolean isEmpty() {
        return false;
    }

    public void fill() {
        BlockSource.fill(this.blocks);
    }
}

