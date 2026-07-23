/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.nbt.NbtCompound;

public class WorldProperties {
    private long seed;
    private int spawnX;
    private int spawnY;
    private int spawnZ;
    private long time;
    private long lastPlayed;
    private long sizeOnDisk;
    private NbtCompound playerNbt;
    private int dimensionId;
    private String name;
    private int version;
    private boolean raining;
    private int rainTime;
    private boolean thundering;
    private int thunderTime;

    public WorldProperties(NbtCompound nbt) {
        this.seed = nbt.getLong("RandomSeed");
        this.spawnX = nbt.getInt("SpawnX");
        this.spawnY = nbt.getInt("SpawnY");
        this.spawnZ = nbt.getInt("SpawnZ");
        this.time = nbt.getLong("Time");
        this.lastPlayed = nbt.getLong("LastPlayed");
        this.sizeOnDisk = nbt.getLong("SizeOnDisk");
        this.name = nbt.getString("LevelName");
        this.version = nbt.getInt("version");
        this.rainTime = nbt.getInt("rainTime");
        this.raining = nbt.getBoolean("raining");
        this.thunderTime = nbt.getInt("thunderTime");
        this.thundering = nbt.getBoolean("thundering");
        if (nbt.contains("Player")) {
            this.playerNbt = nbt.getCompound("Player");
            this.dimensionId = this.playerNbt.getInt("Dimension");
        }
    }

    public WorldProperties(long seed, String name) {
        this.seed = seed;
        this.name = name;
    }

    public WorldProperties(WorldProperties properties) {
        this.seed = properties.seed;
        this.spawnX = properties.spawnX;
        this.spawnY = properties.spawnY;
        this.spawnZ = properties.spawnZ;
        this.time = properties.time;
        this.lastPlayed = properties.lastPlayed;
        this.sizeOnDisk = properties.sizeOnDisk;
        this.playerNbt = properties.playerNbt;
        this.dimensionId = properties.dimensionId;
        this.name = properties.name;
        this.version = properties.version;
        this.rainTime = properties.rainTime;
        this.raining = properties.raining;
        this.thunderTime = properties.thunderTime;
        this.thundering = properties.thundering;
    }

    public NbtCompound asNbt() {
        NbtCompound nbtCompound = new NbtCompound();
        this.updateProperties(nbtCompound, this.playerNbt);
        return nbtCompound;
    }

    public NbtCompound asNbt(List players) {
        NbtCompound nbtCompound = new NbtCompound();
        PlayerEntity playerEntity = null;
        NbtCompound nbtCompound2 = null;
        if (players.size() > 0) {
            playerEntity = (PlayerEntity)players.get(0);
        }
        if (playerEntity != null) {
            nbtCompound2 = new NbtCompound();
            playerEntity.write(nbtCompound2);
        }
        this.updateProperties(nbtCompound, nbtCompound2);
        return nbtCompound;
    }

    private void updateProperties(NbtCompound nbt, NbtCompound playerNbt) {
        nbt.putLong("RandomSeed", this.seed);
        nbt.putInt("SpawnX", this.spawnX);
        nbt.putInt("SpawnY", this.spawnY);
        nbt.putInt("SpawnZ", this.spawnZ);
        nbt.putLong("Time", this.time);
        nbt.putLong("SizeOnDisk", this.sizeOnDisk);
        nbt.putLong("LastPlayed", System.currentTimeMillis());
        nbt.putString("LevelName", this.name);
        nbt.putInt("version", this.version);
        nbt.putInt("rainTime", this.rainTime);
        nbt.putBoolean("raining", this.raining);
        nbt.putInt("thunderTime", this.thunderTime);
        nbt.putBoolean("thundering", this.thundering);
        if (playerNbt != null) {
            nbt.put("Player", playerNbt);
        }
    }

    public long getSeed() {
        return this.seed;
    }

    public int getSpawnX() {
        return this.spawnX;
    }

    public int getSpawnY() {
        return this.spawnY;
    }

    public int getSpawnZ() {
        return this.spawnZ;
    }

    public long getTime() {
        return this.time;
    }

    public long getSizeOnDisk() {
        return this.sizeOnDisk;
    }

    @Environment(value=EnvType.CLIENT)
    public NbtCompound getPlayerNbt() {
        return this.playerNbt;
    }

    public int getDimensionId() {
        return this.dimensionId;
    }

    @Environment(value=EnvType.CLIENT)
    public void setSpawnX(int spawnX) {
        this.spawnX = spawnX;
    }

    @Environment(value=EnvType.CLIENT)
    public void setSpawnY(int spawnY) {
        this.spawnY = spawnY;
    }

    @Environment(value=EnvType.CLIENT)
    public void setSpawnZ(int spawnZ) {
        this.spawnZ = spawnZ;
    }

    public void setTime(long time) {
        this.time = time;
    }

    public void setSizeOnDisk(long sizeOnDisk) {
        this.sizeOnDisk = sizeOnDisk;
    }

    @Environment(value=EnvType.CLIENT)
    public void setPlayerNbt(NbtCompound playerNbt) {
        this.playerNbt = playerNbt;
    }

    public void setSpawn(int x, int y, int z) {
        this.spawnX = x;
        this.spawnY = y;
        this.spawnZ = z;
    }

    @Environment(value=EnvType.CLIENT)
    public String getName() {
        return this.name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int getVersion() {
        return this.version;
    }

    public void setVersion(int version) {
        this.version = version;
    }

    @Environment(value=EnvType.CLIENT)
    public long setLastPlayed() {
        return this.lastPlayed;
    }

    public boolean getThundering() {
        return this.thundering;
    }

    public void setThundering(boolean thundering) {
        this.thundering = thundering;
    }

    public int getThunderTime() {
        return this.thunderTime;
    }

    public void setThunderTime(int thunderTime) {
        this.thunderTime = thunderTime;
    }

    public boolean getRaining() {
        return this.raining;
    }

    public void setRaining(boolean raining) {
        this.raining = raining;
    }

    public int getRainTime() {
        return this.rainTime;
    }

    public void setRainTime(int rainTime) {
        this.rainTime = rainTime;
    }
}

