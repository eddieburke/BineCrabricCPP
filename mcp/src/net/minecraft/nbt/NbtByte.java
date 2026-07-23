/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtByte
extends NbtElement {
    public byte value;

    public NbtByte() {
    }

    public NbtByte(byte value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeByte(this.value);
    }

    void read(DataInput input) {
        this.value = input.readByte();
    }

    public byte getType() {
        return 1;
    }

    public String toString() {
        return "" + this.value;
    }
}

