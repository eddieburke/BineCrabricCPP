/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.storage;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtIo;
import net.minecraft.nbt.NbtShort;
import net.minecraft.world.PersistentState;
import net.minecraft.world.storage.WorldStorage;

public class SavedDataStorage {
    private WorldStorage storage;
    private Map loadedStatesById = new HashMap();
    private List loadedStates = new ArrayList();
    private Map idCounts = new HashMap();

    public SavedDataStorage(WorldStorage storage) {
        this.storage = storage;
        this.loadIdCounts();
    }

    public PersistentState getOrCreate(Class stateClass, String id) {
        PersistentState persistentState;
        block7: {
            persistentState = (PersistentState)this.loadedStatesById.get(id);
            if (persistentState != null) {
                return persistentState;
            }
            if (this.storage != null) {
                try {
                    File file = this.storage.getWorldPropertiesFile(id);
                    if (file == null || !file.exists()) break block7;
                    try {
                        persistentState = (PersistentState)stateClass.getConstructor(String.class).newInstance(id);
                    }
                    catch (Exception exception) {
                        throw new RuntimeException("Failed to instantiate " + stateClass.toString(), exception);
                    }
                    FileInputStream fileInputStream = new FileInputStream(file);
                    NbtCompound nbtCompound = NbtIo.readCompressed(fileInputStream);
                    fileInputStream.close();
                    persistentState.readNbt(nbtCompound.getCompound("data"));
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                }
            }
        }
        if (persistentState != null) {
            this.loadedStatesById.put(id, persistentState);
            this.loadedStates.add(persistentState);
        }
        return persistentState;
    }

    public void set(String id, PersistentState state) {
        if (state == null) {
            throw new RuntimeException("Can't set null data");
        }
        if (this.loadedStatesById.containsKey(id)) {
            this.loadedStates.remove(this.loadedStatesById.remove(id));
        }
        this.loadedStatesById.put(id, state);
        this.loadedStates.add(state);
    }

    public void save() {
        for (int i = 0; i < this.loadedStates.size(); ++i) {
            PersistentState persistentState = (PersistentState)this.loadedStates.get(i);
            if (!persistentState.isDirty()) continue;
            this.save(persistentState);
            persistentState.setDirty(false);
        }
    }

    private void save(PersistentState state) {
        if (this.storage == null) {
            return;
        }
        try {
            File file = this.storage.getWorldPropertiesFile(state.id);
            if (file != null) {
                NbtCompound nbtCompound = new NbtCompound();
                state.writeNbt(nbtCompound);
                NbtCompound nbtCompound2 = new NbtCompound();
                nbtCompound2.put("data", nbtCompound);
                FileOutputStream fileOutputStream = new FileOutputStream(file);
                NbtIo.writeCompressed(nbtCompound2, fileOutputStream);
                fileOutputStream.close();
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    private void loadIdCounts() {
        try {
            this.idCounts.clear();
            if (this.storage == null) {
                return;
            }
            File file = this.storage.getWorldPropertiesFile("idcounts");
            if (file != null && file.exists()) {
                DataInputStream dataInputStream = new DataInputStream(new FileInputStream(file));
                NbtCompound nbtCompound = NbtIo.read(dataInputStream);
                dataInputStream.close();
                for (NbtElement nbtElement : nbtCompound.values()) {
                    if (!(nbtElement instanceof NbtShort)) continue;
                    NbtShort nbtShort = (NbtShort)nbtElement;
                    String string = nbtShort.getKey();
                    short s = nbtShort.value;
                    this.idCounts.put(string, s);
                }
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public int getIdCount(String id) {
        Object object;
        Comparable<Short> comparable;
        Short s = (Short)this.idCounts.get(id);
        if (s == null) {
            s = 0;
        } else {
            comparable = s;
            s = (short)(s + 1);
            object = s;
        }
        this.idCounts.put(id, s);
        if (this.storage == null) {
            return s.shortValue();
        }
        try {
            comparable = this.storage.getWorldPropertiesFile("idcounts");
            if (comparable != null) {
                object = new NbtCompound();
                for (String string : this.idCounts.keySet()) {
                    short s2 = (Short)this.idCounts.get(string);
                    ((NbtCompound)object).putShort(string, s2);
                }
                DataOutputStream dataOutputStream = new DataOutputStream(new FileOutputStream((File)comparable));
                NbtIo.write((NbtCompound)object, dataOutputStream);
                dataOutputStream.close();
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        return s.shortValue();
    }
}

