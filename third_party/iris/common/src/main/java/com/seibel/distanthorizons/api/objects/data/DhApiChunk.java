/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.data;

import com.seibel.distanthorizons.api.objects.data.DhApiTerrainDataPoint;
import java.util.ArrayList;
import java.util.List;

public class DhApiChunk {
    public final int chunkPosX;
    public final int chunkPosZ;
    public final int bottomYBlockPos;
    public final int topYBlockPos;
    private final List<List<DhApiTerrainDataPoint>> dataPoints;

    @Deprecated
    public DhApiChunk(int chunkPosX, int chunkPosZ, int topYBlockPos, int bottomYBlockPos) {
        this(chunkPosX, chunkPosZ, bottomYBlockPos, topYBlockPos, false);
    }

    public static DhApiChunk create(int chunkPosX, int chunkPosZ, int bottomYBlockPos, int topYBlockPos) {
        return new DhApiChunk(chunkPosX, chunkPosZ, bottomYBlockPos, topYBlockPos, false);
    }

    private DhApiChunk(int chunkPosX, int chunkPosZ, int bottomYBlockPos, int topYBlockPos, boolean ignoredParameter) {
        this.chunkPosX = chunkPosX;
        this.chunkPosZ = chunkPosZ;
        this.bottomYBlockPos = bottomYBlockPos;
        this.topYBlockPos = topYBlockPos;
        this.dataPoints = new ArrayList<List<DhApiTerrainDataPoint>>(256);
        for (int i = 0; i < 256; ++i) {
            this.dataPoints.add(i, null);
        }
    }

    public List<DhApiTerrainDataPoint> getDataPoints(int relX, int relZ) throws IndexOutOfBoundsException {
        DhApiChunk.throwIfRelativePosOutOfBounds(relX, relZ);
        return this.dataPoints.get(relZ << 4 | relX);
    }

    public void setDataPoints(int relX, int relZ, List<DhApiTerrainDataPoint> dataPoints) throws IndexOutOfBoundsException, IllegalArgumentException {
        DhApiChunk.throwIfRelativePosOutOfBounds(relX, relZ);
        if (dataPoints != null) {
            for (int i = 0; i < dataPoints.size(); ++i) {
                DhApiTerrainDataPoint dataPoint = dataPoints.get(i);
                if (dataPoint == null) {
                    throw new IllegalArgumentException("Null DhApiTerrainDataPoints are not allowed. If you want to represent empty terrain, please use AIR.");
                }
                if (dataPoint.detailLevel == 0) continue;
                throw new IllegalArgumentException("DhApiTerrainDataPoints has the wrong detail level [" + dataPoint.detailLevel + "], all data points must be block sized; IE their detail level must be [0].");
            }
        }
        this.dataPoints.set(relZ << 4 | relX, dataPoints);
    }

    private static void throwIfRelativePosOutOfBounds(int relX, int relZ) {
        if (relX < 0 || relX > 15 || relZ < 0 || relZ > 15) {
            throw new IndexOutOfBoundsException("Relative block positions must be between 0 and 15 (inclusive) the block pos: (" + relX + "," + relZ + ") is outside of those boundaries.");
        }
    }
}

