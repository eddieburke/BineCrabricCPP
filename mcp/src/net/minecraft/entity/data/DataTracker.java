/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.data;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.data.DataTrackerEntry;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.Packet;
import net.minecraft.util.math.Vec3i;

public class DataTracker {
    private static final HashMap DATA_TYPES = new HashMap();
    private final Map entries = new HashMap();
    private boolean dirty;

    public void startTracking(int key, Object value) {
        Integer n = (Integer)DATA_TYPES.get(value.getClass());
        if (n == null) {
            throw new IllegalArgumentException("Unknown data type: " + value.getClass());
        }
        if (key > 31) {
            throw new IllegalArgumentException("Data value id is too big with " + key + "! (Max is " + 31 + ")");
        }
        if (this.entries.containsKey(key)) {
            throw new IllegalArgumentException("Duplicate id value for " + key + "!");
        }
        DataTrackerEntry dataTrackerEntry = new DataTrackerEntry(n, key, value);
        this.entries.put(key, dataTrackerEntry);
    }

    public byte getByte(int id) {
        return (Byte)((DataTrackerEntry)this.entries.get(id)).get();
    }

    public int getInt(int id) {
        return (Integer)((DataTrackerEntry)this.entries.get(id)).get();
    }

    public String getString(int id) {
        return (String)((DataTrackerEntry)this.entries.get(id)).get();
    }

    public void set(int id, Object object) {
        DataTrackerEntry dataTrackerEntry = (DataTrackerEntry)this.entries.get(id);
        if (!object.equals(dataTrackerEntry.get())) {
            dataTrackerEntry.set(object);
            dataTrackerEntry.setDirty(true);
            this.dirty = true;
        }
    }

    @Environment(value=EnvType.SERVER)
    public boolean isDirty() {
        return this.dirty;
    }

    public static void writeEntries(List entries, DataOutputStream output) {
        if (entries != null) {
            for (DataTrackerEntry dataTrackerEntry : entries) {
                DataTracker.writeEntry(output, dataTrackerEntry);
            }
        }
        output.writeByte(127);
    }

    @Environment(value=EnvType.SERVER)
    public ArrayList getDirtyEntries() {
        ArrayList<DataTrackerEntry> arrayList = null;
        if (this.dirty) {
            for (DataTrackerEntry dataTrackerEntry : this.entries.values()) {
                if (!dataTrackerEntry.isDirty()) continue;
                dataTrackerEntry.setDirty(false);
                if (arrayList == null) {
                    arrayList = new ArrayList<DataTrackerEntry>();
                }
                arrayList.add(dataTrackerEntry);
            }
        }
        this.dirty = false;
        return arrayList;
    }

    public void writeAllEntries(DataOutputStream output) {
        for (DataTrackerEntry dataTrackerEntry : this.entries.values()) {
            DataTracker.writeEntry(output, dataTrackerEntry);
        }
        output.writeByte(127);
    }

    private static void writeEntry(DataOutputStream output, DataTrackerEntry entry) {
        int n = (entry.getDataTypeId() << 5 | entry.getId() & 0x1F) & 0xFF;
        output.writeByte(n);
        switch (entry.getDataTypeId()) {
            case 0: {
                output.writeByte(((Byte)entry.get()).byteValue());
                break;
            }
            case 1: {
                output.writeShort(((Short)entry.get()).shortValue());
                break;
            }
            case 2: {
                output.writeInt((Integer)entry.get());
                break;
            }
            case 3: {
                output.writeFloat(((Float)entry.get()).floatValue());
                break;
            }
            case 4: {
                Packet.writeString((String)entry.get(), output);
                break;
            }
            case 5: {
                ItemStack itemStack = (ItemStack)entry.get();
                output.writeShort(itemStack.getItem().id);
                output.writeByte(itemStack.count);
                output.writeShort(itemStack.getDamage());
                break;
            }
            case 6: {
                Vec3i vec3i = (Vec3i)entry.get();
                output.writeInt(vec3i.x);
                output.writeInt(vec3i.y);
                output.writeInt(vec3i.z);
            }
        }
    }

    public static List readEntries(DataInputStream input) {
        ArrayList<DataTrackerEntry> arrayList = null;
        byte by = input.readByte();
        while (by != 127) {
            if (arrayList == null) {
                arrayList = new ArrayList<DataTrackerEntry>();
            }
            int n = (by & 0xE0) >> 5;
            int n2 = by & 0x1F;
            DataTrackerEntry dataTrackerEntry = null;
            switch (n) {
                case 0: {
                    dataTrackerEntry = new DataTrackerEntry(n, n2, input.readByte());
                    break;
                }
                case 1: {
                    dataTrackerEntry = new DataTrackerEntry(n, n2, input.readShort());
                    break;
                }
                case 2: {
                    dataTrackerEntry = new DataTrackerEntry(n, n2, input.readInt());
                    break;
                }
                case 3: {
                    dataTrackerEntry = new DataTrackerEntry(n, n2, Float.valueOf(input.readFloat()));
                    break;
                }
                case 4: {
                    dataTrackerEntry = new DataTrackerEntry(n, n2, Packet.readString(input, 64));
                    break;
                }
                case 5: {
                    int n3 = input.readShort();
                    int n4 = input.readByte();
                    int n5 = input.readShort();
                    dataTrackerEntry = new DataTrackerEntry(n, n2, new ItemStack(n3, n4, n5));
                    break;
                }
                case 6: {
                    int n3 = input.readInt();
                    int n4 = input.readInt();
                    int n5 = input.readInt();
                    dataTrackerEntry = new DataTrackerEntry(n, n2, new Vec3i(n3, n4, n5));
                }
            }
            arrayList.add(dataTrackerEntry);
            by = input.readByte();
        }
        return arrayList;
    }

    @Environment(value=EnvType.CLIENT)
    public void writeUpdatedEntries(List entries) {
        for (DataTrackerEntry dataTrackerEntry : entries) {
            DataTrackerEntry dataTrackerEntry2 = (DataTrackerEntry)this.entries.get(dataTrackerEntry.getId());
            if (dataTrackerEntry2 == null) continue;
            dataTrackerEntry2.set(dataTrackerEntry.get());
        }
    }

    static {
        DATA_TYPES.put(Byte.class, 0);
        DATA_TYPES.put(Short.class, 1);
        DATA_TYPES.put(Integer.class, 2);
        DATA_TYPES.put(Float.class, 3);
        DATA_TYPES.put(String.class, 4);
        DATA_TYPES.put(ItemStack.class, 5);
        DATA_TYPES.put(Vec3i.class, 6);
    }
}

