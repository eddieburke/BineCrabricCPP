/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtIo;
import net.minecraft.world.World;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.storage.AlphaChunkStorage;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.chunk.storage.RegionIo;

public class RegionChunkStorage
implements ChunkStorage {
    private static final int QUEUE_CAPACITY = 1024;
    private static final SaveRequest POISON = new SaveRequest(null, 0, 0, null, null);

    private final File dir;
    private final BlockingQueue<SaveRequest> saveQueue = new ArrayBlockingQueue<SaveRequest>(QUEUE_CAPACITY);
    private final Thread writerThread;

    public RegionChunkStorage(File dir) {
        this.dir = dir;
        this.writerThread = new Thread(this::writerLoop, "chunk-writer");
        this.writerThread.setDaemon(true);
        this.writerThread.start();
    }

    private void writerLoop() {
        while (true) {
            try {
                SaveRequest req = saveQueue.take();
                if (req == POISON) break;
                try {
                    DataOutputStream out = RegionIo.getChunkOutputStream(req.dir, req.chunkX, req.chunkZ);
                    out.write(req.data, 0, req.data.length);
                    out.close();
                    WorldProperties props = req.world.getProperties();
                    props.setSizeOnDisk(props.getSizeOnDisk() + (long)RegionIo.getChunkSize(req.dir, req.chunkX, req.chunkZ));
                } catch (Exception e) {
                    e.printStackTrace();
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }
    }

    public Chunk loadChunk(World world, int chunkX, int chunkZ) {
        DataInputStream dataInputStream = RegionIo.getChunkInputStream(this.dir, chunkX, chunkZ);
        if (dataInputStream == null) {
            return null;
        }
        NbtCompound nbtCompound = NbtIo.read(dataInputStream);
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

    public void saveChunk(World world, Chunk chunk) {
        world.checkSessionLock();
        try {
            // Serialize on the calling thread — no I/O, just memory writes.
            NbtCompound nbtCompound = new NbtCompound();
            NbtCompound nbtCompound2 = new NbtCompound();
            nbtCompound.put("Level", (NbtElement)nbtCompound2);
            AlphaChunkStorage.saveChunkToNbt(chunk, world, nbtCompound2);

            ByteArrayOutputStream baos = new ByteArrayOutputStream(65536);
            NbtIo.write(nbtCompound, new DataOutputStream(baos));
            byte[] data = baos.toByteArray();

            SaveRequest req = new SaveRequest(this.dir, chunk.x, chunk.z, world, data);
            saveQueue.put(req);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void saveEntities(World world, Chunk chunk) {
    }

    public void tick() {
    }

    public void flush() {
        // Drain queue synchronously so world-close waits for all pending writes.
        try {
            saveQueue.put(POISON);
            writerThread.join();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private static class SaveRequest {
        final File dir;
        final int chunkX, chunkZ;
        final World world;
        final byte[] data;

        SaveRequest(File dir, int chunkX, int chunkZ, World world, byte[] data) {
            this.dir = dir;
            this.chunkX = chunkX;
            this.chunkZ = chunkZ;
            this.world = world;
            this.data = data;
        }
    }
}
