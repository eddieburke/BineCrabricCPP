/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtDouble
extends NbtElement {
    public double value;

    public NbtDouble() {
    }

    public NbtDouble(double value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeDouble(this.value);
    }

    void read(DataInput input) {
        this.value = input.readDouble();
    }

    public byte getType() {
        return 6;
    }

    public String toString() {
        return "" + this.value;
    }
}

