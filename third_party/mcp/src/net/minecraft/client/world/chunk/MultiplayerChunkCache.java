/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.world.chunk;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.EmptyChunk;

@Environment(value=EnvType.CLIENT)
public class MultiplayerChunkCache
implements ChunkSource {
    private Chunk empty;
    private Map chunksByPos = new HashMap();
    private List chunks = new ArrayList();
    private World world;

    public MultiplayerChunkCache(World world) {
        this.empty = new EmptyChunk(world, new byte[32768], 0, 0);
        this.world = world;
    }

    public boolean isChunkLoaded(int x, int z) {
        if (this != null) {
            return true;
        }
        ChunkPos chunkPos = new ChunkPos(x, z);
        return this.chunksByPos.containsKey(chunkPos);
    }

    public void unloadChunk(int chunkX, int chunkY) {
        Chunk chunk = this.getChunk(chunkX, chunkY);
        if (!chunk.isEmpty()) {
            chunk.unload();
        }
        this.chunksByPos.remove(new ChunkPos(chunkX, chunkY));
        this.chunks.remove(chunk);
    }

    public Chunk loadChunk(int chunkX, int chunkZ) {
        ChunkPos chunkPos = new ChunkPos(chunkX, chunkZ);
        byte[] byArray = new byte[32768];
        Chunk chunk = new Chunk(this.world, byArray, chunkX, chunkZ);
        Arrays.fill(chunk.skyLight.bytes, (byte)-1);
        this.chunksByPos.put(chunkPos, chunk);
        chunk.loaded = true;
        return chunk;
    }

    public Chunk getChunk(int chunkX, int chunkZ) {
        ChunkPos chunkPos = new ChunkPos(chunkX, chunkZ);
        Chunk chunk = (Chunk)this.chunksByPos.get(chunkPos);
        if (chunk == null) {
            return this.empty;
        }
        return chunk;
    }

    public boolean save(boolean saveEntities, LoadingDisplay display) {
        return true;
    }

    public boolean tick() {
        return false;
    }

    public boolean canSave() {
        return false;
    }

    public void decorate(ChunkSource source, int x, int z) {
    }

    public String getDebugInfo() {
        return "MultiplayerChunkCache: " + this.chunksByPos.size();
    }
}

