/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.storage;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtIo;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.storage.AlphaWorldStorage;
import net.minecraft.world.storage.WorldSaveInfo;
import net.minecraft.world.storage.WorldStorage;
import net.minecraft.world.storage.WorldStorageSource;

public class AlphaWorldStorageSource
implements WorldStorageSource {
    protected final File dir;

    public AlphaWorldStorageSource(File dir) {
        if (!dir.exists()) {
            dir.mkdirs();
        }
        this.dir = dir;
    }

    @Environment(value=EnvType.CLIENT)
    public String getName() {
        return "Old Format";
    }

    @Environment(value=EnvType.CLIENT)
    public List getAll() {
        ArrayList<WorldSaveInfo> arrayList = new ArrayList<WorldSaveInfo>();
        for (int i = 0; i < 5; ++i) {
            String string = "World" + (i + 1);
            WorldProperties worldProperties = this.getWorldProperties(string);
            if (worldProperties == null) continue;
            arrayList.add(new WorldSaveInfo(string, "", worldProperties.setLastPlayed(), worldProperties.getSizeOnDisk(), false));
        }
        return arrayList;
    }

    @Environment(value=EnvType.CLIENT)
    public void flush() {
    }

    public WorldProperties getWorldProperties(String string) {
        File file = new File(this.dir, string);
        if (!file.exists()) {
            return null;
        }
        File file2 = new File(file, "level.dat");
        if (file2.exists()) {
            try {
                NbtCompound nbtCompound = NbtIo.readCompressed(new FileInputStream(file2));
                NbtCompound nbtCompound2 = nbtCompound.getCompound("Data");
                return new WorldProperties(nbtCompound2);
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        if ((file2 = new File(file, "level.dat_old")).exists()) {
            try {
                NbtCompound nbtCompound = NbtIo.readCompressed(new FileInputStream(file2));
                NbtCompound nbtCompound3 = nbtCompound.getCompound("Data");
                return new WorldProperties(nbtCompound3);
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public void rename(String saveName, String newName) {
        File file = new File(this.dir, saveName);
        if (!file.exists()) {
            return;
        }
        File file2 = new File(file, "level.dat");
        if (file2.exists()) {
            try {
                NbtCompound nbtCompound = NbtIo.readCompressed(new FileInputStream(file2));
                NbtCompound nbtCompound2 = nbtCompound.getCompound("Data");
                nbtCompound2.putString("LevelName", newName);
                NbtIo.writeCompressed(nbtCompound, new FileOutputStream(file2));
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void delete(String saveName) {
        File file = new File(this.dir, saveName);
        if (!file.exists()) {
            return;
        }
        AlphaWorldStorageSource.deleteFilesAndDirs(file.listFiles());
        file.delete();
    }

    protected static void deleteFilesAndDirs(File[] files) {
        for (int i = 0; i < files.length; ++i) {
            if (files[i].isDirectory()) {
                AlphaWorldStorageSource.deleteFilesAndDirs(files[i].listFiles());
            }
            files[i].delete();
        }
    }

    public WorldStorage getSaveLoader(String string, boolean bl) {
        return new AlphaWorldStorage(this.dir, string, bl);
    }

    public boolean needsConversion(String saveName) {
        return false;
    }

    public boolean convert(String saveName, LoadingDisplay display) {
        return false;
    }
}

