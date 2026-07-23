/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import java.io.File;
import java.util.regex.Matcher;
import net.minecraft.world.storage.DataFilenameFilter;

class DataFile
implements Comparable {
    private final File file;
    private final int chunkX;
    private final int chunkZ;

    public DataFile(File file) {
        this.file = file;
        Matcher matcher = DataFilenameFilter.PATTERN.matcher(file.getName());
        if (matcher.matches()) {
            this.chunkX = Integer.parseInt(matcher.group(1), 36);
            this.chunkZ = Integer.parseInt(matcher.group(2), 36);
        } else {
            this.chunkX = 0;
            this.chunkZ = 0;
        }
    }

    public int compareTo(DataFile arg) {
        int n = this.chunkX >> 5;
        int n2 = arg.chunkX >> 5;
        if (n == n2) {
            int n3 = this.chunkZ >> 5;
            int n4 = arg.chunkZ >> 5;
            return n3 - n4;
        }
        return n - n2;
    }

    public File getFile() {
        return this.file;
    }

    public int getChunkX() {
        return this.chunkX;
    }

    public int getChunkZ() {
        return this.chunkZ;
    }
}

