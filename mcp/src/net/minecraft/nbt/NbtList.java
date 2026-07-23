/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.nbt.NbtElement;

public class NbtList
extends NbtElement {
    private List value = new ArrayList();
    private byte type;

    void write(DataOutput output) {
        this.type = this.value.size() > 0 ? ((NbtElement)this.value.get(0)).getType() : (byte)1;
        output.writeByte(this.type);
        output.writeInt(this.value.size());
        for (int i = 0; i < this.value.size(); ++i) {
            ((NbtElement)this.value.get(i)).write(output);
        }
    }

    void read(DataInput input) {
        this.type = input.readByte();
        int n = input.readInt();
        this.value = new ArrayList();
        for (int i = 0; i < n; ++i) {
            NbtElement nbtElement = NbtElement.createTypeFromId(this.type);
            nbtElement.read(input);
            this.value.add(nbtElement);
        }
    }

    public byte getType() {
        return 9;
    }

    public String toString() {
        return "" + this.value.size() + " entries of type " + NbtElement.getTypeNameFromId(this.type);
    }

    public void add(NbtElement element) {
        this.type = element.getType();
        this.value.add(element);
    }

    public NbtElement get(int index) {
        return (NbtElement)this.value.get(index);
    }

    public int size() {
        return this.value.size();
    }
}

