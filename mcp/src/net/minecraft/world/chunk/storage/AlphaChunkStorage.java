/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityRegistry;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtIo;
import net.minecraft.nbt.NbtList;
import net.minecraft.world.World;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkNibbleArray;
import net.minecraft.world.chunk.storage.ChunkStorage;

public class AlphaChunkStorage
implements ChunkStorage {
    private File dir;
    private boolean make;

    public AlphaChunkStorage(File dir, boolean make) {
        this.dir = dir;
        this.make = make;
    }

    private File getChunkFile(int chunkX, int chunkZ) {
        String string = "c." + Integer.toString(chunkX, 36) + "." + Integer.toString(chunkZ, 36) + ".dat";
        String string2 = Integer.toString(chunkX & 0x3F, 36);
        String string3 = Integer.toString(chunkZ & 0x3F, 36);
        File file = new File(this.dir, string2);
        if (!file.exists()) {
            if (this.make) {
                file.mkdir();
            } else {
                return null;
            }
        }
        if (!(file = new File(file, string3)).exists()) {
            if (this.make) {
                file.mkdir();
            } else {
                return null;
            }
        }
        if (!(file = new File(file, string)).exists() && !this.make) {
            return null;
        }
        return file;
    }

    public Chunk loadChunk(World world, int chunkX, int chunkZ) {
        File file = this.getChunkFile(chunkX, chunkZ);
        if (file != null && file.exists()) {
            try {
                FileInputStream fileInputStream = new FileInputStream(file);
                NbtCompound nbtCompound = NbtIo.readCompressed(fileInputStream);
                if (!nbtCompound.contains("Level")) {
                    System.out.println("Chunk file at " + chunkX + "," + chunkZ + " is missing level data, skipping");
                    return null;
                }
                if (!nbtCompound.getCompound("Level").contains("Blocks")) {
                    System.out.println("Chunk file at " + chunkX + "," + chunkZ + " is missing block data, skipping");
                    return null;
                }
                Chunk chunk = AlphaChunkStorage.loadChunkFromNbt(world, nbtCompound.getCompound("Level"));
                if (!chunk.chunkPosEquals(chunkX, chunkZ)) {
                    System.out.println("Chunk file at " + chunkX + "," + chunkZ + " is in the wrong location; relocating. (Expected " + chunkX + ", " + chunkZ + ", got " + chunk.x + ", " + chunk.z + ")");
                    nbtCompound.putInt("xPos", chunkX);
                    nbtCompound.putInt("zPos", chunkZ);
                    chunk = AlphaChunkStorage.loadChunkFromNbt(world, nbtCompound.getCompound("Level"));
                }
                chunk.fill();
                return chunk;
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        return null;
    }

    public void saveChunk(World world, Chunk chunk) {
        Object object;
        world.checkSessionLock();
        File file = this.getChunkFile(chunk.x, chunk.z);
        if (file.exists()) {
            object = world.getProperties();
            ((WorldProperties)object).setSizeOnDisk(((WorldProperties)object).getSizeOnDisk() - file.length());
        }
        try {
            object = new File(this.dir, "tmp_chunk.dat");
            FileOutputStream fileOutputStream = new FileOutputStream((File)object);
            NbtCompound nbtCompound = new NbtCompound();
            NbtCompound nbtCompound2 = new NbtCompound();
            nbtCompound.put("Level", (NbtElement)nbtCompound2);
            AlphaChunkStorage.saveChunkToNbt(chunk, world, nbtCompound2);
            NbtIo.writeCompressed(nbtCompound, fileOutputStream);
            fileOutputStream.close();
            if (file.exists()) {
                file.delete();
            }
            ((File)object).renameTo(file);
            WorldProperties worldProperties = world.getProperties();
            worldProperties.setSizeOnDisk(worldProperties.getSizeOnDisk() + file.length());
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public static void saveChunkToNbt(Chunk chunk, World world, NbtCompound nbt) {
        NbtCompound nbtCompound;
        world.checkSessionLock();
        nbt.putInt("xPos", chunk.x);
        nbt.putInt("zPos", chunk.z);
        nbt.putLong("LastUpdate", world.getTime());
        nbt.putByteArray("Blocks", chunk.blocks);
        nbt.putByteArray("Data", chunk.meta.bytes);
        nbt.putByteArray("SkyLight", chunk.skyLight.bytes);
        nbt.putByteArray("BlockLight", chunk.blockLight.bytes);
        nbt.putByteArray("HeightMap", chunk.heightmap);
        nbt.putBoolean("TerrainPopulated", chunk.terrainPopulated);
        chunk.lastSaveHadEntities = false;
        NbtList nbtList = new NbtList();
        for (int i = 0; i < chunk.entities.length; ++i) {
            for (Object object : chunk.entities[i]) {
                chunk.lastSaveHadEntities = true;
                nbtCompound = new NbtCompound();
                if (!((Entity)object).saveSelfNbt(nbtCompound)) continue;
                nbtList.add(nbtCompound);
            }
        }
        nbt.put("Entities", nbtList);
        NbtList nbtList2 = new NbtList();
        for (Object object : chunk.blockEntities.values()) {
            nbtCompound = new NbtCompound();
            ((BlockEntity)object).writeNbt(nbtCompound);
            nbtList2.add(nbtCompound);
        }
        nbt.put("TileEntities", nbtList2);
    }

    public static Chunk loadChunkFromNbt(World world, NbtCompound nbt) {
        NbtList nbtList;
        Object object;
        NbtList nbtList2;
        int n = nbt.getInt("xPos");
        int n2 = nbt.getInt("zPos");
        Chunk chunk = new Chunk(world, n, n2);
        chunk.blocks = nbt.getByteArray("Blocks");
        chunk.meta = new ChunkNibbleArray(nbt.getByteArray("Data"));
        chunk.skyLight = new ChunkNibbleArray(nbt.getByteArray("SkyLight"));
        chunk.blockLight = new ChunkNibbleArray(nbt.getByteArray("BlockLight"));
        chunk.heightmap = nbt.getByteArray("HeightMap");
        chunk.terrainPopulated = nbt.getBoolean("TerrainPopulated");
        if (!chunk.meta.isArrayInitialized()) {
            chunk.meta = new ChunkNibbleArray(chunk.blocks.length);
        }
        if (chunk.heightmap == null || !chunk.skyLight.isArrayInitialized()) {
            chunk.heightmap = new byte[256];
            chunk.skyLight = new ChunkNibbleArray(chunk.blocks.length);
            chunk.populateHeightMap();
        }
        if (!chunk.blockLight.isArrayInitialized()) {
            chunk.blockLight = new ChunkNibbleArray(chunk.blocks.length);
            chunk.onLoad();
        }
        if ((nbtList2 = nbt.getList("Entities")) != null) {
            for (int i = 0; i < nbtList2.size(); ++i) {
                NbtCompound nbtCompound = (NbtCompound)nbtList2.get(i);
                object = EntityRegistry.getEntityFromNbt(nbtCompound, world);
                chunk.lastSaveHadEntities = true;
                if (object == null) continue;
                chunk.addEntity((Entity)object);
            }
        }
        if ((nbtList = nbt.getList("TileEntities")) != null) {
            for (int i = 0; i < nbtList.size(); ++i) {
                object = (NbtCompound)nbtList.get(i);
                BlockEntity blockEntity = BlockEntity.createFromNbt((NbtCompound)object);
                if (blockEntity == null) continue;
                chunk.addBlockEntity(blockEntity);
            }
        }
        return chunk;
    }

    public void tick() {
    }

    public void flush() {
    }

    public void saveEntities(World world, Chunk chunk) {
    }
}

