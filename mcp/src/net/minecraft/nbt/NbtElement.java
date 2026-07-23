/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataOutput;
import net.minecraft.nbt.NbtByte;
import net.minecraft.nbt.NbtByteArray;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtDouble;
import net.minecraft.nbt.NbtEnd;
import net.minecraft.nbt.NbtFloat;
import net.minecraft.nbt.NbtInt;
import net.minecraft.nbt.NbtList;
import net.minecraft.nbt.NbtLong;
import net.minecraft.nbt.NbtShort;
import net.minecraft.nbt.NbtString;

public abstract class NbtElement {
    private String key = null;

    abstract void write(DataOutput var1);

    abstract void read(DataInput var1);

    public abstract byte getType();

    public String getKey() {
        if (this.key == null) {
            return "";
        }
        return this.key;
    }

    public NbtElement setKey(String key) {
        this.key = key;
        return this;
    }

    public static NbtElement readTag(DataInput input) {
        byte by = input.readByte();
        if (by == 0) {
            return new NbtEnd();
        }
        NbtElement nbtElement = NbtElement.createTypeFromId(by);
        nbtElement.key = input.readUTF();
        nbtElement.read(input);
        return nbtElement;
    }

    public static void writeTag(NbtElement element, DataOutput output) {
        output.writeByte(element.getType());
        if (element.getType() == 0) {
            return;
        }
        output.writeUTF(element.getKey());
        element.write(output);
    }

    public static NbtElement createTypeFromId(byte id) {
        switch (id) {
            case 0: {
                return new NbtEnd();
            }
            case 1: {
                return new NbtByte();
            }
            case 2: {
                return new NbtShort();
            }
            case 3: {
                return new NbtInt();
            }
            case 4: {
                return new NbtLong();
            }
            case 5: {
                return new NbtFloat();
            }
            case 6: {
                return new NbtDouble();
            }
            case 7: {
                return new NbtByteArray();
            }
            case 8: {
                return new NbtString();
            }
            case 9: {
                return new NbtList();
            }
            case 10: {
                return new NbtCompound();
            }
        }
        return null;
    }

    public static String getTypeNameFromId(byte id) {
        switch (id) {
            case 0: {
                return "TAG_End";
            }
            case 1: {
                return "TAG_Byte";
            }
            case 2: {
                return "TAG_Short";
            }
            case 3: {
                return "TAG_Int";
            }
            case 4: {
                return "TAG_Long";
            }
            case 5: {
                return "TAG_Float";
            }
            case 6: {
                return "TAG_Double";
            }
            case 7: {
                return "TAG_Byte_Array";
            }
            case 8: {
                return "TAG_String";
            }
            case 9: {
                return "TAG_List";
            }
            case 10: {
                return "TAG_Compound";
            }
        }
        return "UNKNOWN";
    }
}

