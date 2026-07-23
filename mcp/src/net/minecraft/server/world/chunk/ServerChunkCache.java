/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.world.chunk;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.EmptyChunk;
import net.minecraft.world.chunk.storage.ChunkStorage;

@Environment(value=EnvType.SERVER)
public class ServerChunkCache
implements ChunkSource {
    private Set chunksToUnload = new HashSet();
    private Chunk empty;
    private ChunkSource generator;
    private ChunkStorage storage;
    public boolean forceLoad = false;
    private Map chunksByPos = new HashMap();
    private List chunks = new ArrayList();
    private ServerWorld world;

    public ServerChunkCache(ServerWorld world, ChunkStorage storage, ChunkSource generator) {
        this.empty = new EmptyChunk(world, new byte[32768], 0, 0);
        this.world = world;
        this.storage = storage;
        this.generator = generator;
    }

    public boolean isChunkLoaded(int x, int z) {
        return this.chunksByPos.containsKey(ChunkPos.hashCode(x, z));
    }

    public void isLoaded(int chunkX, int chunkZ) {
        Vec3i vec3i = this.world.getSpawnPos();
        int n = chunkX * 16 + 8 - vec3i.x;
        int n2 = chunkZ * 16 + 8 - vec3i.z;
        int n3 = 128;
        if (n < -n3 || n > n3 || n2 < -n3 || n2 > n3) {
            this.chunksToUnload.add(ChunkPos.hashCode(chunkX, chunkZ));
        }
    }

    public Chunk loadChunk(int chunkX, int chunkZ) {
        int n = ChunkPos.hashCode(chunkX, chunkZ);
        this.chunksToUnload.remove(n);
        Chunk chunk = (Chunk)this.chunksByPos.get(n);
        if (chunk == null) {
            chunk = this.loadChunkFromStorage(chunkX, chunkZ);
            if (chunk == null) {
                chunk = this.generator == null ? this.empty : this.generator.getChunk(chunkX, chunkZ);
            }
            this.chunksByPos.put(n, chunk);
            this.chunks.add(chunk);
            if (chunk != null) {
                chunk.populateBlockLight();
                chunk.load();
            }
            if (!chunk.terrainPopulated && this.isChunkLoaded(chunkX + 1, chunkZ + 1) && this.isChunkLoaded(chunkX, chunkZ + 1) && this.isChunkLoaded(chunkX + 1, chunkZ)) {
                this.decorate(this, chunkX, chunkZ);
            }
            if (this.isChunkLoaded(chunkX - 1, chunkZ) && !this.getChunk((int)(chunkX - 1), (int)chunkZ).terrainPopulated && this.isChunkLoaded(chunkX - 1, chunkZ + 1) && this.isChunkLoaded(chunkX, chunkZ + 1) && this.isChunkLoaded(chunkX - 1, chunkZ)) {
                this.decorate(this, chunkX - 1, chunkZ);
            }
            if (this.isChunkLoaded(chunkX, chunkZ - 1) && !this.getChunk((int)chunkX, (int)(chunkZ - 1)).terrainPopulated && this.isChunkLoaded(chunkX + 1, chunkZ - 1) && this.isChunkLoaded(chunkX, chunkZ - 1) && this.isChunkLoaded(chunkX + 1, chunkZ)) {
                this.decorate(this, chunkX, chunkZ - 1);
            }
            if (this.isChunkLoaded(chunkX - 1, chunkZ - 1) && !this.getChunk((int)(chunkX - 1), (int)(chunkZ - 1)).terrainPopulated && this.isChunkLoaded(chunkX - 1, chunkZ - 1) && this.isChunkLoaded(chunkX, chunkZ - 1) && this.isChunkLoaded(chunkX - 1, chunkZ)) {
                this.decorate(this, chunkX - 1, chunkZ - 1);
            }
        }
        return chunk;
    }

    public Chunk getChunk(int chunkX, int chunkZ) {
        Chunk chunk = (Chunk)this.chunksByPos.get(ChunkPos.hashCode(chunkX, chunkZ));
        if (chunk == null) {
            if (this.world.eventProcessingEnabled || this.forceLoad) {
                return this.loadChunk(chunkX, chunkZ);
            }
            return this.empty;
        }
        return chunk;
    }

    private Chunk loadChunkFromStorage(int chunkX, int chunkZ) {
        if (this.storage == null) {
            return null;
        }
        try {
            Chunk chunk = this.storage.loadChunk(this.world, chunkX, chunkZ);
            if (chunk != null) {
                chunk.lastSaveTime = this.world.getTime();
            }
            return chunk;
        }
        catch (Exception exception) {
            exception.printStackTrace();
            return null;
        }
    }

    private void saveEntities(Chunk chunk) {
        if (this.storage == null) {
            return;
        }
        try {
            this.storage.saveEntities(this.world, chunk);
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    private void saveChunk(Chunk chunk) {
        if (this.storage == null) {
            return;
        }
        try {
            chunk.lastSaveTime = this.world.getTime();
            this.storage.saveChunk(this.world, chunk);
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
    }

    public void decorate(ChunkSource source, int x, int z) {
        Chunk chunk = this.getChunk(x, z);
        if (!chunk.terrainPopulated) {
            chunk.terrainPopulated = true;
            if (this.generator != null) {
                this.generator.decorate(source, x, z);
                chunk.markDirty();
            }
        }
    }

    public boolean save(boolean saveEntities, LoadingDisplay display) {
        int n = 0;
        for (int i = 0; i < this.chunks.size(); ++i) {
            Chunk chunk = (Chunk)this.chunks.get(i);
            if (saveEntities && !chunk.empty) {
                this.saveEntities(chunk);
            }
            if (!chunk.shouldSave(saveEntities)) continue;
            this.saveChunk(chunk);
            chunk.dirty = false;
            if (++n != 24 || saveEntities) continue;
            return false;
        }
        if (saveEntities) {
            if (this.storage == null) {
                return true;
            }
            this.storage.flush();
        }
        return true;
    }

    public boolean tick() {
        if (!this.world.savingDisabled) {
            for (int i = 0; i < 100; ++i) {
                if (this.chunksToUnload.isEmpty()) continue;
                Integer n = (Integer)this.chunksToUnload.iterator().next();
                Chunk chunk = (Chunk)this.chunksByPos.get(n);
                chunk.unload();
                this.saveChunk(chunk);
                this.saveEntities(chunk);
                this.chunksToUnload.remove(n);
                this.chunksByPos.remove(n);
                this.chunks.remove(chunk);
            }
            if (this.storage != null) {
                this.storage.tick();
            }
        }
        return this.generator.tick();
    }

    public boolean canSave() {
        return !this.world.savingDisabled;
    }
}

