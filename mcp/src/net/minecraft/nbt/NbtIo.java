/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.nbt;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtElement;

public class NbtIo {
    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public static NbtCompound readCompressed(InputStream stream) {
        DataInputStream dataInputStream = new DataInputStream(new GZIPInputStream(stream));
        try {
            NbtCompound nbtCompound = NbtIo.read(dataInputStream);
            return nbtCompound;
        }
        finally {
            dataInputStream.close();
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public static void writeCompressed(NbtCompound nbt, OutputStream stream) {
        DataOutputStream dataOutputStream = new DataOutputStream(new GZIPOutputStream(stream));
        try {
            NbtIo.write(nbt, dataOutputStream);
        }
        finally {
            dataOutputStream.close();
        }
    }

    public static NbtCompound read(DataInput stream) {
        NbtElement nbtElement = NbtElement.readTag(stream);
        if (nbtElement instanceof NbtCompound) {
            return (NbtCompound)nbtElement;
        }
        throw new IOException("Root tag must be a named compound tag");
    }

    public static void write(NbtCompound nbt, DataOutput output) {
        NbtElement.writeTag(nbt, output);
    }
}

