/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.storage;

import java.io.File;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.server.world.PlayerSaveHandler;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.dimension.Dimension;

public interface WorldStorage {
    public WorldProperties loadProperties();

    public void checkSessionLock();

    public ChunkStorage getChunkStorage(Dimension var1);

    public void save(WorldProperties var1, List var2);

    public void save(WorldProperties var1);

    @Environment(value=EnvType.SERVER)
    public PlayerSaveHandler getPlayerSaveHandler();

    @Environment(value=EnvType.SERVER)
    public void forceSave();

    public File getWorldPropertiesFile(String var1);
}

