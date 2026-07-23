/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.storage;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.zip.GZIPInputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.chunk.storage.DataFile;
import net.minecraft.world.chunk.storage.RegionFile;
import net.minecraft.world.chunk.storage.RegionIo;
import net.minecraft.world.storage.AlphaWorldStorageSource;
import net.minecraft.world.storage.DataFilenameFilter;
import net.minecraft.world.storage.DimensionFileFilter;
import net.minecraft.world.storage.RegionWorldStorage;
import net.minecraft.world.storage.WorldSaveInfo;
import net.minecraft.world.storage.WorldStorage;

public class RegionWorldStorageSource
extends AlphaWorldStorageSource {
    public RegionWorldStorageSource(File file) {
        super(file);
    }

    @Environment(value=EnvType.CLIENT)
    public String getName() {
        return "Scaevolus' McRegion";
    }

    @Environment(value=EnvType.CLIENT)
    public List getAll() {
        File[] fileArray;
        ArrayList<WorldSaveInfo> arrayList = new ArrayList<WorldSaveInfo>();
        for (File file : fileArray = this.dir.listFiles()) {
            String string;
            WorldProperties worldProperties;
            if (!file.isDirectory() || (worldProperties = this.getWorldProperties(string = file.getName())) == null) continue;
            boolean bl = worldProperties.getVersion() != 19132;
            String string2 = worldProperties.getName();
            if (string2 == null || MathHelper.isNullOrEmpty(string2)) {
                string2 = string;
            }
            arrayList.add(new WorldSaveInfo(string, string2, worldProperties.setLastPlayed(), worldProperties.getSizeOnDisk(), bl));
        }
        return arrayList;
    }

    @Environment(value=EnvType.CLIENT)
    public void flush() {
        RegionIo.flush();
    }

    public WorldStorage getSaveLoader(String string, boolean bl) {
        return new RegionWorldStorage(this.dir, string, bl);
    }

    public boolean needsConversion(String saveName) {
        WorldProperties worldProperties = this.getWorldProperties(saveName);
        return worldProperties != null && worldProperties.getVersion() == 0;
    }

    public boolean convert(String saveName, LoadingDisplay display) {
        display.progressStagePercentage(0);
        ArrayList arrayList = new ArrayList();
        ArrayList arrayList2 = new ArrayList();
        ArrayList arrayList3 = new ArrayList();
        ArrayList arrayList4 = new ArrayList();
        File file = new File(this.dir, saveName);
        File file2 = new File(file, "DIM-1");
        System.out.println("Scanning folders...");
        this.collectData(file, arrayList, arrayList2);
        if (file2.exists()) {
            this.collectData(file2, arrayList3, arrayList4);
        }
        int n = arrayList.size() + arrayList3.size() + arrayList2.size() + arrayList4.size();
        System.out.println("Total conversion count is " + n);
        this.writeDimensionData(file, arrayList, 0, n, display);
        this.writeDimensionData(file2, arrayList3, arrayList.size(), n, display);
        WorldProperties worldProperties = this.getWorldProperties(saveName);
        worldProperties.setVersion(19132);
        WorldStorage worldStorage = this.getSaveLoader(saveName, false);
        worldStorage.save(worldProperties);
        this.deleteData(arrayList2, arrayList.size() + arrayList3.size(), n, display);
        if (file2.exists()) {
            this.deleteData(arrayList4, arrayList.size() + arrayList3.size() + arrayList2.size(), n, display);
        }
        return true;
    }

    private void collectData(File dir, ArrayList dimensions, ArrayList data) {
        File[] fileArray;
        DimensionFileFilter dimensionFileFilter = new DimensionFileFilter(null);
        DataFilenameFilter dataFilenameFilter = new DataFilenameFilter(null);
        for (File file : fileArray = dir.listFiles(dimensionFileFilter)) {
            File[] fileArray2;
            data.add(file);
            for (File file2 : fileArray2 = file.listFiles(dimensionFileFilter)) {
                File[] fileArray3;
                for (File file3 : fileArray3 = file2.listFiles(dataFilenameFilter)) {
                    dimensions.add(new DataFile(file3));
                }
            }
        }
    }

    private void writeDimensionData(File dir, ArrayList datafiles, int index, int total, LoadingDisplay display) {
        Collections.sort(datafiles);
        byte[] byArray = new byte[4096];
        for (DataFile dataFile : datafiles) {
            int n;
            int n2 = dataFile.getChunkX();
            RegionFile regionFile = RegionIo.getRegionFile(dir, n2, n = dataFile.getChunkZ());
            if (!regionFile.hasChunkData(n2 & 0x1F, n & 0x1F)) {
                try {
                    DataInputStream dataInputStream = new DataInputStream(new GZIPInputStream(new FileInputStream(dataFile.getFile())));
                    DataOutputStream dataOutputStream = regionFile.getChunkOutputStream(n2 & 0x1F, n & 0x1F);
                    int n3 = 0;
                    while ((n3 = dataInputStream.read(byArray)) != -1) {
                        dataOutputStream.write(byArray, 0, n3);
                    }
                    dataOutputStream.close();
                    dataInputStream.close();
                }
                catch (IOException iOException) {
                    iOException.printStackTrace();
                }
            }
            int n4 = (int)Math.round(100.0 * (double)(++index) / (double)total);
            display.progressStagePercentage(n4);
        }
        RegionIo.flush();
    }

    private void deleteData(ArrayList files, int index, int total, LoadingDisplay display) {
        for (File file : files) {
            File[] fileArray = file.listFiles();
            RegionWorldStorageSource.deleteFilesAndDirs(fileArray);
            file.delete();
            int n = (int)Math.round(100.0 * (double)(++index) / (double)total);
            display.progressStagePercentage(n);
        }
    }
}

