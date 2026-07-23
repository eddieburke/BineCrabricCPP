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
import net.minecraft.util.LongObjectHashMapEntry;

@Environment(value=EnvType.SERVER)
public class LongObjectHashMap {
    private transient LongObjectHashMapEntry[] entries = new LongObjectHashMapEntry[16];
    private transient int size;
    private int capacity = 12;
    private final float loadFactor;
    private volatile transient int modCount;

    public LongObjectHashMap() {
        this.loadFactor = 0.75f;
    }

    private static int hash(long key) {
        return LongObjectHashMap.hash((int)(key ^ key >>> 32));
    }

    private static int hash(int key) {
        key ^= key >>> 20 ^ key >>> 12;
        return key ^ key >>> 7 ^ key >>> 4;
    }

    private static int indexOf(int hash, int arrayLength) {
        return hash & arrayLength - 1;
    }

    public Object get(long key) {
        int n = LongObjectHashMap.hash(key);
        LongObjectHashMapEntry longObjectHashMapEntry = this.entries[LongObjectHashMap.indexOf(n, this.entries.length)];
        while (longObjectHashMapEntry != null) {
            if (longObjectHashMapEntry.key == key) {
                return longObjectHashMapEntry.value;
            }
            longObjectHashMapEntry = longObjectHashMapEntry.next;
        }
        return null;
    }

    public void put(long key, Object value) {
        int n = LongObjectHashMap.hash(key);
        int n2 = LongObjectHashMap.indexOf(n, this.entries.length);
        LongObjectHashMapEntry longObjectHashMapEntry = this.entries[n2];
        while (longObjectHashMapEntry != null) {
            if (longObjectHashMapEntry.key == key) {
                longObjectHashMapEntry.value = value;
            }
            longObjectHashMapEntry = longObjectHashMapEntry.next;
        }
        ++this.modCount;
        this.addEntry(n, key, value, n2);
    }

    private void resize(int size) {
        LongObjectHashMapEntry[] longObjectHashMapEntryArray = this.entries;
        int n = longObjectHashMapEntryArray.length;
        if (n == 0x40000000) {
            this.capacity = Integer.MAX_VALUE;
            return;
        }
        LongObjectHashMapEntry[] longObjectHashMapEntryArray2 = new LongObjectHashMapEntry[size];
        this.transfer(longObjectHashMapEntryArray2);
        this.entries = longObjectHashMapEntryArray2;
        this.capacity = (int)((float)size * this.loadFactor);
    }

    private void transfer(LongObjectHashMapEntry[] entryArray) {
        LongObjectHashMapEntry[] longObjectHashMapEntryArray = this.entries;
        int n = entryArray.length;
        for (int i = 0; i < longObjectHashMapEntryArray.length; ++i) {
            LongObjectHashMapEntry longObjectHashMapEntry;
            LongObjectHashMapEntry longObjectHashMapEntry2 = longObjectHashMapEntryArray[i];
            if (longObjectHashMapEntry2 == null) continue;
            longObjectHashMapEntryArray[i] = null;
            do {
                longObjectHashMapEntry = longObjectHashMapEntry2.next;
                int n2 = LongObjectHashMap.indexOf(longObjectHashMapEntry2.hash, n);
                longObjectHashMapEntry2.next = entryArray[n2];
                entryArray[n2] = longObjectHashMapEntry2;
            } while ((longObjectHashMapEntry2 = longObjectHashMapEntry) != null);
        }
    }

    public Object remove(long key) {
        LongObjectHashMapEntry longObjectHashMapEntry = this.removeEntry(key);
        return longObjectHashMapEntry == null ? null : longObjectHashMapEntry.value;
    }

    final LongObjectHashMapEntry removeEntry(long key) {
        LongObjectHashMapEntry longObjectHashMapEntry;
        int n = LongObjectHashMap.hash(key);
        int n2 = LongObjectHashMap.indexOf(n, this.entries.length);
        LongObjectHashMapEntry longObjectHashMapEntry2 = longObjectHashMapEntry = this.entries[n2];
        while (longObjectHashMapEntry2 != null) {
            LongObjectHashMapEntry longObjectHashMapEntry3 = longObjectHashMapEntry2.next;
            if (longObjectHashMapEntry2.key == key) {
                ++this.modCount;
                --this.size;
                if (longObjectHashMapEntry == longObjectHashMapEntry2) {
                    this.entries[n2] = longObjectHashMapEntry3;
                } else {
                    longObjectHashMapEntry.next = longObjectHashMapEntry3;
                }
                return longObjectHashMapEntry2;
            }
            longObjectHashMapEntry = longObjectHashMapEntry2;
            longObjectHashMapEntry2 = longObjectHashMapEntry3;
        }
        return longObjectHashMapEntry2;
    }

    private void addEntry(int hash, long key, Object value, int index) {
        LongObjectHashMapEntry longObjectHashMapEntry = this.entries[index];
        this.entries[index] = new LongObjectHashMapEntry(hash, key, value, longObjectHashMapEntry);
        if (this.size++ >= this.capacity) {
            this.resize(2 * this.entries.length);
        }
    }

    static /* synthetic */ int synthHash(long l) {
        return LongObjectHashMap.hash(l);
    }
}

