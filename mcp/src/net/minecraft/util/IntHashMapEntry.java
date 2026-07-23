/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util;

import net.minecraft.util.IntHashMap;

class IntHashMapEntry {
    final int key;
    Object value;
    IntHashMapEntry next;
    final int hash;

    IntHashMapEntry(int hash, int key, Object value, IntHashMapEntry next) {
        this.value = value;
        this.next = next;
        this.key = key;
        this.hash = hash;
    }

    public final int getKey() {
        return this.key;
    }

    public final Object getValue() {
        return this.value;
    }

    public final boolean equals(Object object) {
        Object object2;
        Object object3;
        Integer n;
        if (!(object instanceof IntHashMapEntry)) {
            return false;
        }
        IntHashMapEntry intHashMapEntry = (IntHashMapEntry)object;
        Integer n2 = this.getKey();
        return (n2 == (n = Integer.valueOf(intHashMapEntry.getKey())) || n2 != null && ((Object)n2).equals(n)) && ((object3 = this.getValue()) == (object2 = intHashMapEntry.getValue()) || object3 != null && object3.equals(object2));
    }

    public final int hashCode() {
        return IntHashMap.synthHash(this.key);
    }

    public final String toString() {
        return this.getKey() + "=" + this.getValue();
    }
}

