/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtInt
extends NbtElement {
    public int value;

    public NbtInt() {
    }

    public NbtInt(int value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeInt(this.value);
    }

    void read(DataInput input) {
        this.value = input.readInt();
    }

    public byte getType() {
        return 3;
    }

    public String toString() {
        return "" + this.value;
    }
}

