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
import net.minecraft.util.LongObjectHashMap;

@Environment(value=EnvType.SERVER)
class LongObjectHashMapEntry {
    final long key;
    Object value;
    LongObjectHashMapEntry next;
    final int hash;

    LongObjectHashMapEntry(int hash, long key, Object value, LongObjectHashMapEntry next) {
        this.value = value;
        this.next = next;
        this.key = key;
        this.hash = hash;
    }

    public final long getKey() {
        return this.key;
    }

    public final Object getValue() {
        return this.value;
    }

    public final boolean equals(Object object) {
        Object object2;
        Object object3;
        Long l;
        if (!(object instanceof LongObjectHashMapEntry)) {
            return false;
        }
        LongObjectHashMapEntry longObjectHashMapEntry = (LongObjectHashMapEntry)object;
        Long l2 = this.getKey();
        return (l2 == (l = Long.valueOf(longObjectHashMapEntry.getKey())) || l2 != null && ((Object)l2).equals(l)) && ((object3 = this.getValue()) == (object2 = longObjectHashMapEntry.getValue()) || object3 != null && object3.equals(object2));
    }

    public final int hashCode() {
        return LongObjectHashMap.synthHash(this.key);
    }

    public final String toString() {
        return this.getKey() + "=" + this.getValue();
    }
}

