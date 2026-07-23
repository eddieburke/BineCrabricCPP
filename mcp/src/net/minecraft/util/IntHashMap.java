/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.util;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.IntHashMapEntry;

public class IntHashMap {
    private transient IntHashMapEntry[] table = new IntHashMapEntry[16];
    private transient int size;
    private int threshold = 12;
    private final float loadFactor;
    private volatile transient int modCount;

    public IntHashMap() {
        this.loadFactor = 0.75f;
    }

    private static int hash(int key) {
        key ^= key >>> 20 ^ key >>> 12;
        return key ^ key >>> 7 ^ key >>> 4;
    }

    private static int indexOf(int hash, int arrayLength) {
        return hash & arrayLength - 1;
    }

    public Object get(int key) {
        int n = IntHashMap.hash(key);
        IntHashMapEntry intHashMapEntry = this.table[IntHashMap.indexOf(n, this.table.length)];
        while (intHashMapEntry != null) {
            if (intHashMapEntry.key == key) {
                return intHashMapEntry.value;
            }
            intHashMapEntry = intHashMapEntry.next;
        }
        return null;
    }

    @Environment(value=EnvType.SERVER)
    public boolean containsKey(int key) {
        return this.getEntry(key) != null;
    }

    @Environment(value=EnvType.SERVER)
    final IntHashMapEntry getEntry(int key) {
        int n = IntHashMap.hash(key);
        IntHashMapEntry intHashMapEntry = this.table[IntHashMap.indexOf(n, this.table.length)];
        while (intHashMapEntry != null) {
            if (intHashMapEntry.key == key) {
                return intHashMapEntry;
            }
            intHashMapEntry = intHashMapEntry.next;
        }
        return null;
    }

    public void put(int key, Object value) {
        int n = IntHashMap.hash(key);
        int n2 = IntHashMap.indexOf(n, this.table.length);
        IntHashMapEntry intHashMapEntry = this.table[n2];
        while (intHashMapEntry != null) {
            if (intHashMapEntry.key == key) {
                intHashMapEntry.value = value;
            }
            intHashMapEntry = intHashMapEntry.next;
        }
        ++this.modCount;
        this.addEntry(n, key, value, n2);
    }

    private void resize(int size) {
        IntHashMapEntry[] intHashMapEntryArray = this.table;
        int n = intHashMapEntryArray.length;
        if (n == 0x40000000) {
            this.threshold = Integer.MAX_VALUE;
            return;
        }
        IntHashMapEntry[] intHashMapEntryArray2 = new IntHashMapEntry[size];
        this.transfer(intHashMapEntryArray2);
        this.table = intHashMapEntryArray2;
        this.threshold = (int)((float)size * this.loadFactor);
    }

    private void transfer(IntHashMapEntry[] entryArray) {
        IntHashMapEntry[] intHashMapEntryArray = this.table;
        int n = entryArray.length;
        for (int i = 0; i < intHashMapEntryArray.length; ++i) {
            IntHashMapEntry intHashMapEntry;
            IntHashMapEntry intHashMapEntry2 = intHashMapEntryArray[i];
            if (intHashMapEntry2 == null) continue;
            intHashMapEntryArray[i] = null;
            do {
                intHashMapEntry = intHashMapEntry2.next;
                int n2 = IntHashMap.indexOf(intHashMapEntry2.hash, n);
                intHashMapEntry2.next = entryArray[n2];
                entryArray[n2] = intHashMapEntry2;
            } while ((intHashMapEntry2 = intHashMapEntry) != null);
        }
    }

    public Object remove(int key) {
        IntHashMapEntry intHashMapEntry = this.removeEntry(key);
        return intHashMapEntry == null ? null : intHashMapEntry.value;
    }

    final IntHashMapEntry removeEntry(int key) {
        IntHashMapEntry intHashMapEntry;
        int n = IntHashMap.hash(key);
        int n2 = IntHashMap.indexOf(n, this.table.length);
        IntHashMapEntry intHashMapEntry2 = intHashMapEntry = this.table[n2];
        while (intHashMapEntry2 != null) {
            IntHashMapEntry intHashMapEntry3 = intHashMapEntry2.next;
            if (intHashMapEntry2.key == key) {
                ++this.modCount;
                --this.size;
                if (intHashMapEntry == intHashMapEntry2) {
                    this.table[n2] = intHashMapEntry3;
                } else {
                    intHashMapEntry.next = intHashMapEntry3;
                }
                return intHashMapEntry2;
            }
            intHashMapEntry = intHashMapEntry2;
            intHashMapEntry2 = intHashMapEntry3;
        }
        return intHashMapEntry2;
    }

    public void clear() {
        ++this.modCount;
        IntHashMapEntry[] intHashMapEntryArray = this.table;
        for (int i = 0; i < intHashMapEntryArray.length; ++i) {
            intHashMapEntryArray[i] = null;
        }
        this.size = 0;
    }

    private void addEntry(int hash, int key, Object value, int index) {
        IntHashMapEntry intHashMapEntry = this.table[index];
        this.table[index] = new IntHashMapEntry(hash, key, value, intHashMapEntry);
        if (this.size++ >= this.threshold) {
            this.resize(2 * this.table.length);
        }
    }

    static /* synthetic */ int synthHash(int key) {
        return IntHashMap.hash(key);
    }
}

