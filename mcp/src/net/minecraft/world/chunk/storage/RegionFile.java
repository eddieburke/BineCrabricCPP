/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.storage;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.zip.Deflater;
import java.util.zip.DeflaterOutputStream;
import java.util.zip.GZIPInputStream;
import java.util.zip.InflaterInputStream;

public class RegionFile {
    private static final byte[] BLOCK_BUFFER = new byte[4096];
    private final File file;
    private RandomAccessFile randomAccessFile;
    private final int[] chunkBlockInfo = new int[1024];
    private final int[] chunkSaveTimes = new int[1024];
    private ArrayList blockEmptyFlags;
    private int bytesWritten;
    private long lastModifiedTime = 0L;

    public RegionFile(File file) {
        this.file = file;
        this.println("REGION LOAD " + this.file);
        this.bytesWritten = 0;
        try {
            int n;
            int n2;
            int n3;
            if (file.exists()) {
                this.lastModifiedTime = file.lastModified();
            }
            this.randomAccessFile = new RandomAccessFile(file, "rw");
            if (this.randomAccessFile.length() < 4096L) {
                for (n3 = 0; n3 < 1024; ++n3) {
                    this.randomAccessFile.writeInt(0);
                }
                for (n3 = 0; n3 < 1024; ++n3) {
                    this.randomAccessFile.writeInt(0);
                }
                this.bytesWritten += 8192;
            }
            if ((this.randomAccessFile.length() & 0xFFFL) != 0L) {
                n3 = 0;
                while ((long)n3 < (this.randomAccessFile.length() & 0xFFFL)) {
                    this.randomAccessFile.write(0);
                    ++n3;
                }
            }
            n3 = (int)this.randomAccessFile.length() / 4096;
            this.blockEmptyFlags = new ArrayList(n3);
            for (n2 = 0; n2 < n3; ++n2) {
                this.blockEmptyFlags.add(true);
            }
            this.blockEmptyFlags.set(0, false);
            this.blockEmptyFlags.set(1, false);
            this.randomAccessFile.seek(0L);
            for (n2 = 0; n2 < 1024; ++n2) {
                this.chunkBlockInfo[n2] = n = this.randomAccessFile.readInt();
                if (n == 0 || (n >> 8) + (n & 0xFF) > this.blockEmptyFlags.size()) continue;
                for (int i = 0; i < (n & 0xFF); ++i) {
                    this.blockEmptyFlags.set((n >> 8) + i, false);
                }
            }
            for (n2 = 0; n2 < 1024; ++n2) {
                this.chunkSaveTimes[n2] = n = this.randomAccessFile.readInt();
            }
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
    }

    public synchronized int resetBytesWritten() {
        int n = this.bytesWritten;
        this.bytesWritten = 0;
        return n;
    }

    private void print(String s) {
    }

    private void println(String s) {
        this.print(s + "\n");
    }

    private void print(String action, int chunkX, int chunkZ, String s) {
        this.print("REGION " + action + " " + this.file.getName() + "[" + chunkX + "," + chunkZ + "] = " + s);
    }

    private void print(String action, int chunkX, int chunkZ, int size, String s) {
        this.print("REGION " + action + " " + this.file.getName() + "[" + chunkX + "," + chunkZ + "] " + size + "B = " + s);
    }

    private void println(String action, int chunkX, int chunkZ, String s) {
        this.print(action, chunkX, chunkZ, s + "\n");
    }

    public synchronized DataInputStream getChunkInputStream(int chunkX, int chunkZ) {
        if (this.isOutsideRegion(chunkX, chunkZ)) {
            this.println("READ", chunkX, chunkZ, "out of bounds");
            return null;
        }
        try {
            int n = this.getChunkBlockInfo(chunkX, chunkZ);
            if (n == 0) {
                return null;
            }
            int n2 = n >> 8;
            int n3 = n & 0xFF;
            if (n2 + n3 > this.blockEmptyFlags.size()) {
                this.println("READ", chunkX, chunkZ, "invalid sector");
                return null;
            }
            this.randomAccessFile.seek(n2 * 4096);
            int n4 = this.randomAccessFile.readInt();
            if (n4 > 4096 * n3) {
                this.println("READ", chunkX, chunkZ, "invalid length: " + n4 + " > 4096 * " + n3);
                return null;
            }
            byte by = this.randomAccessFile.readByte();
            if (by == 1) {
                byte[] byArray = new byte[n4 - 1];
                this.randomAccessFile.read(byArray);
                DataInputStream dataInputStream = new DataInputStream(new GZIPInputStream(new ByteArrayInputStream(byArray)));
                return dataInputStream;
            }
            if (by == 2) {
                byte[] byArray = new byte[n4 - 1];
                this.randomAccessFile.read(byArray);
                DataInputStream dataInputStream = new DataInputStream(new InflaterInputStream(new ByteArrayInputStream(byArray)));
                return dataInputStream;
            }
            this.println("READ", chunkX, chunkZ, "unknown version " + by);
            return null;
        }
        catch (IOException iOException) {
            this.println("READ", chunkX, chunkZ, "exception");
            return null;
        }
    }

    public DataOutputStream getChunkOutputStream(int chunkX, int chunkZ) {
        if (this.isOutsideRegion(chunkX, chunkZ)) {
            return null;
        }
        return new DataOutputStream(new DeflaterOutputStream(new ChunkOutputStream(chunkX, chunkZ), new Deflater(Deflater.BEST_SPEED)));
    }

    protected synchronized void writeChunkData(int chunkX, int chunkZ, byte[] data, int size) {
        try {
            int n = this.getChunkBlockInfo(chunkX, chunkZ);
            int n2 = n >> 8;
            int n3 = n & 0xFF;
            int n4 = (size + 5) / 4096 + 1;
            if (n4 >= 256) {
                return;
            }
            if (n2 != 0 && n3 == n4) {
                this.print("SAVE", chunkX, chunkZ, size, "rewrite");
                this.writeChunkData(n2, data, size);
            } else {
                int n5;
                int n6;
                for (n6 = 0; n6 < n3; ++n6) {
                    this.blockEmptyFlags.set(n2 + n6, true);
                }
                n6 = this.blockEmptyFlags.indexOf(true);
                int n7 = 0;
                if (n6 != -1) {
                    for (n5 = n6; n5 < this.blockEmptyFlags.size(); ++n5) {
                        if (n7 != 0) {
                            n7 = ((Boolean)this.blockEmptyFlags.get(n5)).booleanValue() ? ++n7 : 0;
                        } else if (((Boolean)this.blockEmptyFlags.get(n5)).booleanValue()) {
                            n6 = n5;
                            n7 = 1;
                        }
                        if (n7 >= n4) break;
                    }
                }
                if (n7 >= n4) {
                    this.print("SAVE", chunkX, chunkZ, size, "reuse");
                    n2 = n6;
                    this.writeChunkBlockInfo(chunkX, chunkZ, n2 << 8 | n4);
                    for (n5 = 0; n5 < n4; ++n5) {
                        this.blockEmptyFlags.set(n2 + n5, false);
                    }
                    this.writeChunkData(n2, data, size);
                } else {
                    this.print("SAVE", chunkX, chunkZ, size, "grow");
                    this.randomAccessFile.seek(this.randomAccessFile.length());
                    n2 = this.blockEmptyFlags.size();
                    for (n5 = 0; n5 < n4; ++n5) {
                        this.randomAccessFile.write(BLOCK_BUFFER);
                        this.blockEmptyFlags.add(false);
                    }
                    this.bytesWritten += 4096 * n4;
                    this.writeChunkData(n2, data, size);
                    this.writeChunkBlockInfo(chunkX, chunkZ, n2 << 8 | n4);
                }
            }
            this.writeChunkSaveTime(chunkX, chunkZ, (int)(System.currentTimeMillis() / 1000L));
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
    }

    private void writeChunkData(int blockOffset, byte[] data, int size) {
        this.randomAccessFile.seek(blockOffset * 4096);
        this.randomAccessFile.writeInt(size + 1);
        this.randomAccessFile.writeByte(2);
        this.randomAccessFile.write(data, 0, size);
    }

    private boolean isOutsideRegion(int chunkX, int chunkZ) {
        return chunkX < 0 || chunkX >= 32 || chunkZ < 0 || chunkZ >= 32;
    }

    private int getChunkBlockInfo(int chunkX, int chunkZ) {
        return this.chunkBlockInfo[chunkX + chunkZ * 32];
    }

    public boolean hasChunkData(int chunkX, int chunkZ) {
        return this.getChunkBlockInfo(chunkX, chunkZ) != 0;
    }

    private void writeChunkBlockInfo(int chunkX, int chunkZ, int blockInfo) {
        this.chunkBlockInfo[chunkX + chunkZ * 32] = blockInfo;
        this.randomAccessFile.seek((chunkX + chunkZ * 32) * 4);
        this.randomAccessFile.writeInt(blockInfo);
    }

    private void writeChunkSaveTime(int chunkX, int chunkZ, int seconds) {
        this.chunkSaveTimes[chunkX + chunkZ * 32] = seconds;
        this.randomAccessFile.seek(4096 + (chunkX + chunkZ * 32) * 4);
        this.randomAccessFile.writeInt(seconds);
    }

    public void close() {
        this.randomAccessFile.close();
    }

    class ChunkOutputStream
    extends ByteArrayOutputStream {
        private int chunkX;
        private int chunkZ;

        public ChunkOutputStream(int chunkX, int chunkZ) {
            super(8096);
            this.chunkX = chunkX;
            this.chunkZ = chunkZ;
        }

        public void close() {
            RegionFile.this.writeChunkData(this.chunkX, this.chunkZ, this.buf, this.count);
        }
    }
}

