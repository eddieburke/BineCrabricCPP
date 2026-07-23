/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.Map;
import net.minecraft.world.chunk.storage.RegionFile;

public class RegionIo {
    private static final int MAX_OPEN_REGIONS = 32;
    private static final Map<File, RegionFile> REGION_FILES = new LinkedHashMap<File, RegionFile>(MAX_OPEN_REGIONS, 0.75f, true) {
        protected boolean removeEldestEntry(Map.Entry eldest) {
            if (size() > MAX_OPEN_REGIONS) {
                try { ((RegionFile)eldest.getValue()).close(); } catch (IOException e) { e.printStackTrace(); }
                return true;
            }
            return false;
        }
    };

    private RegionIo() {
    }

    public static synchronized RegionFile getRegionFile(File worldDir, int chunkX, int chunkZ) {
        File regionDir = new File(worldDir, "region");
        File file = new File(regionDir, "r." + (chunkX >> 5) + "." + (chunkZ >> 5) + ".mcr");
        RegionFile regionFile = REGION_FILES.get(file);
        if (regionFile != null) {
            return regionFile;
        }
        if (!regionDir.exists()) {
            regionDir.mkdirs();
        }
        regionFile = new RegionFile(file);
        REGION_FILES.put(file, regionFile);
        return regionFile;
    }

    public static synchronized void flush() {
        for (RegionFile regionFile : REGION_FILES.values()) {
            try {
                regionFile.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        REGION_FILES.clear();
    }

    public static int getChunkSize(File worldDir, int chunkX, int chunkZ) {
        RegionFile regionFile = RegionIo.getRegionFile(worldDir, chunkX, chunkZ);
        return regionFile.resetBytesWritten();
    }

    public static DataInputStream getChunkInputStream(File worldDir, int chunkX, int chunkZ) {
        RegionFile regionFile = RegionIo.getRegionFile(worldDir, chunkX, chunkZ);
        return regionFile.getChunkInputStream(chunkX & 0x1F, chunkZ & 0x1F);
    }

    public static DataOutputStream getChunkOutputStream(File worldDir, int chunkX, int chunkZ) {
        RegionFile regionFile = RegionIo.getRegionFile(worldDir, chunkX, chunkZ);
        return regionFile.getChunkOutputStream(chunkX & 0x1F, chunkZ & 0x1F);
    }
}
