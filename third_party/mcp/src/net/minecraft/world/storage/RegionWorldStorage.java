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
import net.minecraft.world.chunk.storage.RegionChunkStorage;
import net.minecraft.world.chunk.storage.RegionIo;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.dimension.NetherDimension;
import net.minecraft.world.storage.AlphaWorldStorage;

public class RegionWorldStorage
extends AlphaWorldStorage {
    public RegionWorldStorage(File file, String string, boolean bl) {
        super(file, string, bl);
    }

    public ChunkStorage getChunkStorage(Dimension dimension) {
        File file = this.getDirectory();
        if (dimension instanceof NetherDimension) {
            File file2 = new File(file, "DIM-1");
            file2.mkdirs();
            return new RegionChunkStorage(file2);
        }
        return new RegionChunkStorage(file);
    }

    public void save(WorldProperties properties, List players) {
        properties.setVersion(19132);
        super.save(properties, players);
    }

    @Environment(value=EnvType.SERVER)
    public void forceSave() {
        RegionIo.flush();
    }
}

