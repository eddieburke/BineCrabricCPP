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
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.storage.WorldStorage;

@Environment(value=EnvType.CLIENT)
public class EmptyWorldStorage
implements WorldStorage {
    public WorldProperties loadProperties() {
        return null;
    }

    public void checkSessionLock() {
    }

    public ChunkStorage getChunkStorage(Dimension dimension) {
        return null;
    }

    public void save(WorldProperties properties, List players) {
    }

    public void save(WorldProperties properties) {
    }

    public File getWorldPropertiesFile(String name) {
        return null;
    }
}

