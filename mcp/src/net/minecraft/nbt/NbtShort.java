/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtShort
extends NbtElement {
    public short value;

    public NbtShort() {
    }

    public NbtShort(short value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeShort(this.value);
    }

    void read(DataInput input) {
        this.value = input.readShort();
    }

    public byte getType() {
        return 2;
    }

    public String toString() {
        return "" + this.value;
    }
}

