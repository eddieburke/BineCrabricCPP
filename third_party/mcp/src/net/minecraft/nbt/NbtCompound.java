/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import net.minecraft.nbt.NbtByte;
import net.minecraft.nbt.NbtByteArray;
import net.minecraft.nbt.NbtDouble;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtFloat;
import net.minecraft.nbt.NbtInt;
import net.minecraft.nbt.NbtList;
import net.minecraft.nbt.NbtLong;
import net.minecraft.nbt.NbtShort;
import net.minecraft.nbt.NbtString;

public class NbtCompound
extends NbtElement {
    private Map entries = new HashMap();

    void write(DataOutput output) {
        for (NbtElement nbtElement : this.entries.values()) {
            NbtElement.writeTag(nbtElement, output);
        }
        output.writeByte(0);
    }

    void read(DataInput input) {
        NbtElement nbtElement;
        this.entries.clear();
        while ((nbtElement = NbtElement.readTag(input)).getType() != 0) {
            this.entries.put(nbtElement.getKey(), nbtElement);
        }
    }

    public Collection values() {
        return this.entries.values();
    }

    public byte getType() {
        return 10;
    }

    public void put(String key, NbtElement value) {
        this.entries.put(key, value.setKey(key));
    }

    public void putByte(String key, byte value) {
        this.entries.put(key, new NbtByte(value).setKey(key));
    }

    public void putShort(String key, short value) {
        this.entries.put(key, new NbtShort(value).setKey(key));
    }

    public void putInt(String key, int value) {
        this.entries.put(key, new NbtInt(value).setKey(key));
    }

    public void putLong(String key, long value) {
        this.entries.put(key, new NbtLong(value).setKey(key));
    }

    public void putFloat(String key, float value) {
        this.entries.put(key, new NbtFloat(value).setKey(key));
    }

    public void putDouble(String key, double value) {
        this.entries.put(key, new NbtDouble(value).setKey(key));
    }

    public void putString(String key, String value) {
        this.entries.put(key, new NbtString(value).setKey(key));
    }

    public void putByteArray(String key, byte[] value) {
        this.entries.put(key, new NbtByteArray(value).setKey(key));
    }

    public void put(String key, NbtCompound value) {
        this.entries.put(key, value.setKey(key));
    }

    public void putBoolean(String key, boolean value) {
        this.putByte(key, value ? (byte)1 : 0);
    }

    public boolean contains(String key) {
        return this.entries.containsKey(key);
    }

    public byte getByte(String key) {
        if (!this.entries.containsKey(key)) {
            return 0;
        }
        return ((NbtByte)this.entries.get((Object)key)).value;
    }

    public short getShort(String key) {
        if (!this.entries.containsKey(key)) {
            return 0;
        }
        return ((NbtShort)this.entries.get((Object)key)).value;
    }

    public int getInt(String key) {
        if (!this.entries.containsKey(key)) {
            return 0;
        }
        return ((NbtInt)this.entries.get((Object)key)).value;
    }

    public long getLong(String key) {
        if (!this.entries.containsKey(key)) {
            return 0L;
        }
        return ((NbtLong)this.entries.get((Object)key)).value;
    }

    public float getFloat(String key) {
        if (!this.entries.containsKey(key)) {
            return 0.0f;
        }
        return ((NbtFloat)this.entries.get((Object)key)).value;
    }

    public double getDouble(String key) {
        if (!this.entries.containsKey(key)) {
            return 0.0;
        }
        return ((NbtDouble)this.entries.get((Object)key)).value;
    }

    public String getString(String key) {
        if (!this.entries.containsKey(key)) {
            return "";
        }
        return ((NbtString)this.entries.get((Object)key)).value;
    }

    public byte[] getByteArray(String key) {
        if (!this.entries.containsKey(key)) {
            return new byte[0];
        }
        return ((NbtByteArray)this.entries.get((Object)key)).value;
    }

    public NbtCompound getCompound(String key) {
        if (!this.entries.containsKey(key)) {
            return new NbtCompound();
        }
        return (NbtCompound)this.entries.get(key);
    }

    public NbtList getList(String key) {
        if (!this.entries.containsKey(key)) {
            return new NbtList();
        }
        return (NbtList)this.entries.get(key);
    }

    public boolean getBoolean(String key) {
        return this.getByte(key) != 0;
    }

    public String toString() {
        return "" + this.entries.size() + " entries";
    }
}

