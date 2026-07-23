/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.storage;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class WorldSaveInfo
implements Comparable {
    private final String saveName;
    private final String name;
    private final long lastPlayed;
    private final long size;
    private final boolean sameVersion;

    public WorldSaveInfo(String fileName, String name, long lastPlayed, long size, boolean sameVersion) {
        this.saveName = fileName;
        this.name = name;
        this.lastPlayed = lastPlayed;
        this.size = size;
        this.sameVersion = sameVersion;
    }

    public String getSaveName() {
        return this.saveName;
    }

    public String getName() {
        return this.name;
    }

    public long getSize() {
        return this.size;
    }

    public boolean isSameVersion() {
        return this.sameVersion;
    }

    public long getLastPlayed() {
        return this.lastPlayed;
    }

    public int compareTo(WorldSaveInfo arg) {
        if (this.lastPlayed < arg.lastPlayed) {
            return 1;
        }
        if (this.lastPlayed > arg.lastPlayed) {
            return -1;
        }
        return this.saveName.compareTo(arg.saveName);
    }
}

