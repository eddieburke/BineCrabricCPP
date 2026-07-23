/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtByteArray
extends NbtElement {
    public byte[] value;

    public NbtByteArray() {
    }

    public NbtByteArray(byte[] value) {
        this.value = value;
    }

    void write(DataOutput output) {
        output.writeInt(this.value.length);
        output.write(this.value);
    }

    void read(DataInput input) {
        int n = input.readInt();
        this.value = new byte[n];
        input.readFully(this.value);
    }

    public byte getType() {
        return 7;
    }

    public String toString() {
        return "[" + this.value.length + " bytes]";
    }
}

