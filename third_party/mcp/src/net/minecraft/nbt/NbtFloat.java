/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtFloat
extends NbtElement {
    public float value;

    public NbtFloat() {
    }

    public NbtFloat(float value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeFloat(this.value);
    }

    void read(DataInput input) {
        this.value = input.readFloat();
    }

    public byte getType() {
        return 5;
    }

    public String toString() {
        return "" + this.value;
    }
}

