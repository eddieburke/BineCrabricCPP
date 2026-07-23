/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtLong
extends NbtElement {
    public long value;

    public NbtLong() {
    }

    public NbtLong(long value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeLong(this.value);
    }

    void read(DataInput input) {
        this.value = input.readLong();
    }

    public byte getType() {
        return 4;
    }

    public String toString() {
        return "" + this.value;
    }
}

