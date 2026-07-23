/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.chunk;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.world.chunk.Chunk;

public interface ChunkSource {
    public boolean isChunkLoaded(int var1, int var2);

    public Chunk getChunk(int var1, int var2);

    public Chunk loadChunk(int var1, int var2);

    public void decorate(ChunkSource var1, int var2, int var3);

    public boolean save(boolean var1, LoadingDisplay var2);

    public boolean tick();

    public boolean canSave();

    @Environment(value=EnvType.CLIENT)
    public String getDebugInfo();
}

