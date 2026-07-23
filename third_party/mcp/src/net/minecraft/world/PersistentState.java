/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world;

import net.minecraft.nbt.NbtCompound;

public abstract class PersistentState {
    public final String id;
    private boolean dirty;

    public PersistentState(String id) {
        this.id = id;
    }

    public abstract void readNbt(NbtCompound var1);

    public abstract void writeNbt(NbtCompound var1);

    public void markDirty() {
        this.setDirty(true);
    }

    public void setDirty(boolean dirty) {
        this.dirty = dirty;
    }

    public boolean isDirty() {
        return this.dirty;
    }
}

