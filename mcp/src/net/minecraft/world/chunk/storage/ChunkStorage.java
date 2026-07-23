/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public interface ChunkStorage {
    public Chunk loadChunk(World var1, int var2, int var3);

    public void saveChunk(World var1, Chunk var2);

    public void saveEntities(World var1, Chunk var2);

    public void tick();

    public void flush();
}

