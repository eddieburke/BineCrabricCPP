/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.data;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

public class DataTrackerEntry {
    private final int dataTypeId;
    private final int id;
    private Object value;
    private boolean dirty;

    public DataTrackerEntry(int dataTypeId, int id, Object value) {
        this.id = id;
        this.value = value;
        this.dataTypeId = dataTypeId;
        this.dirty = true;
    }

    public int getId() {
        return this.id;
    }

    public void set(Object value) {
        this.value = value;
    }

    public Object get() {
        return this.value;
    }

    public int getDataTypeId() {
        return this.dataTypeId;
    }

    @Environment(value=EnvType.SERVER)
    public boolean isDirty() {
        return this.dirty;
    }

    public void setDirty(boolean dirty) {
        this.dirty = dirty;
    }
}

