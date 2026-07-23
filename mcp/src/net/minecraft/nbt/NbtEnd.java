/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtElement;

public class NbtEnd
extends NbtElement {
    void read(DataInput input) {
    }

    void write(DataOutput output) {
    }

    public byte getType() {
        return 0;
    }

    public String toString() {
        return "END";
    }
}

