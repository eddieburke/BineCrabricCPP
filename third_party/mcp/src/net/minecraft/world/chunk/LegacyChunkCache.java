/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.chunk;

import java.io.IOException;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.storage.ChunkStorage;

@Environment(value=EnvType.CLIENT)
public class LegacyChunkCache
implements ChunkSource {
    private Chunk empty;
    private ChunkSource generator;
    private ChunkStorage storage;
    private Chunk[] chunks;
    private World world;
    int cachedChunkX;
    int cachedChunkZ;
    private Chunk cachedChunk;
    private int spawnChunkX;
    private int spawnChunkZ;

    public void setSpawnPoint(int chunkX, int chunkZ) {
        this.spawnChunkX = chunkX;
        this.spawnChunkZ = chunkZ;
    }

    public boolean isSpawnChunk(int chunkX, int chunkZ) {
        int n = 15;
        return chunkX >= this.spawnChunkX - n && chunkZ >= this.spawnChunkZ - n && chunkX <= this.spawnChunkX + n && chunkZ <= this.spawnChunkZ + n;
    }

    public boolean isChunkLoaded(int x, int z) {
        if (!this.isSpawnChunk(x, z)) {
            return false;
        }
        if (x == this.cachedChunkX && z == this.cachedChunkZ && this.cachedChunk != null) {
            return true;
        }
        int n = x & 0x1F;
        int n2 = z & 0x1F;
        int n3 = n + n2 * 32;
        return this.chunks[n3] != null && (this.chunks[n3] == this.empty || this.chunks[n3].chunkPosEquals(x, z));
    }

    public Chunk loadChunk(int chunkX, int chunkZ) {
        return this.getChunk(chunkX, chunkZ);
    }

    public Chunk getChunk(int chunkX, int chunkZ) {
        if (chunkX == this.cachedChunkX && chunkZ == this.cachedChunkZ && this.cachedChunk != null) {
            return this.cachedChunk;
        }
        if (!this.world.eventProcessingEnabled && !this.isSpawnChunk(chunkX, chunkZ)) {
            return this.empty;
        }
        int n = chunkX & 0x1F;
        int n2 = chunkZ & 0x1F;
        int n3 = n + n2 * 32;
        if (!this.isChunkLoaded(chunkX, chunkZ)) {
            Chunk chunk;
            if (this.chunks[n3] != null) {
                this.chunks[n3].unload();
                this.saveChunk(this.chunks[n3]);
                this.saveEntities(this.chunks[n3]);
            }
            if ((chunk = this.loadChunkFromStorage(chunkX, chunkZ)) == null) {
                if (this.generator == null) {
                    chunk = this.empty;
                } else {
                    chunk = this.generator.getChunk(chunkX, chunkZ);
                    chunk.fill();
                }
            }
            this.chunks[n3] = chunk;
            chunk.populateBlockLight();
            if (this.chunks[n3] != null) {
                this.chunks[n3].load();
            }
            if (!this.chunks[n3].terrainPopulated && this.isChunkLoaded(chunkX + 1, chunkZ + 1) && this.isChunkLoaded(chunkX, chunkZ + 1) && this.isChunkLoaded(chunkX + 1, chunkZ)) {
                this.decorate(this, chunkX, chunkZ);
            }
            if (this.isChunkLoaded(chunkX - 1, chunkZ) && !this.getChunk((int)(chunkX - 1), (int)chunkZ).terrainPopulated && this.isChunkLoaded(chunkX - 1, chunkZ + 1) && this.isChunkLoaded(chunkX, chunkZ + 1) && this.isChunkLoaded(chunkX + 1, chunkZ)) {
                this.decorate(this, chunkX - 1, chunkZ);
            }
            if (this.isChunkLoaded(chunkX, chunkZ - 1) && !this.getChunk((int)chunkX, (int)(chunkZ - 1)).terrainPopulated && this.isChunkLoaded(chunkX + 1, chunkZ - 1) && this.isChunkLoaded(chunkX, chunkZ - 1) && this.isChunkLoaded(chunkX + 1, chunkZ)) {
                this.decorate(this, chunkX, chunkZ - 1);
            }
            if (this.isChunkLoaded(chunkX - 1, chunkZ - 1) && !this.getChunk((int)(chunkX - 1), (int)(chunkZ - 1)).terrainPopulated && this.isChunkLoaded(chunkX - 1, chunkZ - 1) && this.isChunkLoaded(chunkX, chunkZ - 1) && this.isChunkLoaded(chunkX - 1, chunkZ)) {
                this.decorate(this, chunkX - 1, chunkZ - 1);
            }
        }
        this.cachedChunkX = chunkX;
        this.cachedChunkZ = chunkZ;
        this.cachedChunk = this.chunks[n3];
        return this.chunks[n3];
    }

    private Chunk loadChunkFromStorage(int chunkX, int chunkZ) {
        if (this.storage == null) {
            return this.empty;
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
            return this.empty;
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
        int n;
        int n2 = 0;
        int n3 = 0;
        if (display != null) {
            for (n = 0; n < this.chunks.length; ++n) {
                if (this.chunks[n] == null || !this.chunks[n].shouldSave(saveEntities)) continue;
                ++n3;
            }
        }
        n = 0;
        for (int i = 0; i < this.chunks.length; ++i) {
            if (this.chunks[i] == null) continue;
            if (saveEntities && !this.chunks[i].empty) {
                this.saveEntities(this.chunks[i]);
            }
            if (!this.chunks[i].shouldSave(saveEntities)) continue;
            this.saveChunk(this.chunks[i]);
            this.chunks[i].dirty = false;
            if (++n2 == 2 && !saveEntities) {
                return false;
            }
            if (display == null || ++n % 10 != 0) continue;
            display.progressStagePercentage(n * 100 / n3);
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
        if (this.storage != null) {
            this.storage.tick();
        }
        return this.generator.tick();
    }

    public boolean canSave() {
        return true;
    }

    public String getDebugInfo() {
        return "ChunkCache: " + this.chunks.length;
    }
}

