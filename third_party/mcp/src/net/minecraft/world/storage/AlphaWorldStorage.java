/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 *  net.fabricmc.api.EnvironmentInterface
 *  net.fabricmc.api.EnvironmentInterfaces
 */
package net.minecraft.world.storage;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.api.EnvironmentInterface;
import net.fabricmc.api.EnvironmentInterfaces;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtIo;
import net.minecraft.server.world.PlayerSaveHandler;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.storage.AlphaChunkStorage;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.dimension.NetherDimension;
import net.minecraft.world.storage.WorldStorage;
import net.minecraft.world.storage.exception.SessionLockException;

@EnvironmentInterfaces(value={@EnvironmentInterface(value=EnvType.SERVER, itf=PlayerSaveHandler.class)})
public class AlphaWorldStorage
implements PlayerSaveHandler,
WorldStorage {
    private static final Logger LOGGER = Logger.getLogger("Minecraft");
    private final File dir;
    private final File playerDataDir;
    private final File dataDir;
    private final long startTime = System.currentTimeMillis();

    public AlphaWorldStorage(File savesDir, String name, boolean createPlayerDataDir) {
        this.dir = new File(savesDir, name);
        this.dir.mkdirs();
        this.playerDataDir = new File(this.dir, "players");
        this.dataDir = new File(this.dir, "data");
        this.dataDir.mkdirs();
        if (createPlayerDataDir) {
            this.playerDataDir.mkdirs();
        }
        this.writeSessionLock();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private void writeSessionLock() {
        try {
            File file = new File(this.dir, "session.lock");
            DataOutputStream dataOutputStream = new DataOutputStream(new FileOutputStream(file));
            try {
                dataOutputStream.writeLong(this.startTime);
            }
            finally {
                dataOutputStream.close();
            }
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
            throw new RuntimeException("Failed to check session lock, aborting");
        }
    }

    protected File getDirectory() {
        return this.dir;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void checkSessionLock() {
        try {
            File file = new File(this.dir, "session.lock");
            DataInputStream dataInputStream = new DataInputStream(new FileInputStream(file));
            try {
                if (dataInputStream.readLong() != this.startTime) {
                    throw new SessionLockException("The save is being accessed from another location, aborting");
                }
            }
            finally {
                dataInputStream.close();
            }
        }
        catch (IOException iOException) {
            throw new SessionLockException("Failed to check session lock, aborting");
        }
    }

    public ChunkStorage getChunkStorage(Dimension dimension) {
        if (dimension instanceof NetherDimension) {
            File file = new File(this.dir, "DIM-1");
            file.mkdirs();
            return new AlphaChunkStorage(file, true);
        }
        return new AlphaChunkStorage(this.dir, true);
    }

    public WorldProperties loadProperties() {
        File file = new File(this.dir, "level.dat");
        if (file.exists()) {
            try {
                NbtCompound nbtCompound = NbtIo.readCompressed(new FileInputStream(file));
                NbtCompound nbtCompound2 = nbtCompound.getCompound("Data");
                return new WorldProperties(nbtCompound2);
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        if ((file = new File(this.dir, "level.dat_old")).exists()) {
            try {
                NbtCompound nbtCompound = NbtIo.readCompressed(new FileInputStream(file));
                NbtCompound nbtCompound3 = nbtCompound.getCompound("Data");
                return new WorldProperties(nbtCompound3);
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        return null;
    }

    public void save(WorldProperties properties, List players) {
        NbtCompound nbtCompound = properties.asNbt(players);
        NbtCompound nbtCompound2 = new NbtCompound();
        nbtCompound2.put("Data", (NbtElement)nbtCompound);
        try {
            File file = new File(this.dir, "level.dat_new");
            File file2 = new File(this.dir, "level.dat_old");
            File file3 = new File(this.dir, "level.dat");
            NbtIo.writeCompressed(nbtCompound2, new FileOutputStream(file));
            if (file2.exists()) {
                file2.delete();
            }
            file3.renameTo(file2);
            if (file3.exists()) {
                file3.delete();
            }
            file.renameTo(file3);
            if (file.exists()) {
                file.delete();
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public void save(WorldProperties properties) {
        NbtCompound nbtCompound = properties.asNbt();
        NbtCompound nbtCompound2 = new NbtCompound();
        nbtCompound2.put("Data", (NbtElement)nbtCompound);
        try {
            File file = new File(this.dir, "level.dat_new");
            File file2 = new File(this.dir, "level.dat_old");
            File file3 = new File(this.dir, "level.dat");
            NbtIo.writeCompressed(nbtCompound2, new FileOutputStream(file));
            if (file2.exists()) {
                file2.delete();
            }
            file3.renameTo(file2);
            if (file3.exists()) {
                file3.delete();
            }
            file.renameTo(file3);
            if (file.exists()) {
                file.delete();
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    @Environment(value=EnvType.SERVER)
    public void savePlayerData(PlayerEntity player) {
        try {
            NbtCompound nbtCompound = new NbtCompound();
            player.write(nbtCompound);
            File file = new File(this.playerDataDir, "_tmp_.dat");
            File file2 = new File(this.playerDataDir, player.name + ".dat");
            NbtIo.writeCompressed(nbtCompound, new FileOutputStream(file));
            if (file2.exists()) {
                file2.delete();
            }
            file.renameTo(file2);
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to save player data for " + player.name);
        }
    }

    @Environment(value=EnvType.SERVER)
    public void loadPlayerData(PlayerEntity player) {
        NbtCompound nbtCompound = this.loadPlayerData(player.name);
        if (nbtCompound != null) {
            player.read(nbtCompound);
        }
    }

    @Environment(value=EnvType.SERVER)
    public NbtCompound loadPlayerData(String playerName) {
        try {
            File file = new File(this.playerDataDir, playerName + ".dat");
            if (file.exists()) {
                return NbtIo.readCompressed(new FileInputStream(file));
            }
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to load player data for " + playerName);
        }
        return null;
    }

    @Environment(value=EnvType.SERVER)
    public PlayerSaveHandler getPlayerSaveHandler() {
        return this;
    }

    @Environment(value=EnvType.SERVER)
    public void forceSave() {
    }

    public File getWorldPropertiesFile(String name) {
        return new File(this.dataDir, name + ".dat");
    }
}

