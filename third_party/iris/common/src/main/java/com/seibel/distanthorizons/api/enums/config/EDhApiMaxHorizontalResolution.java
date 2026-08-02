/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

import com.seibel.distanthorizons.coreapi.util.MathUtil;
import java.util.ArrayList;
import java.util.Collections;

public enum EDhApiMaxHorizontalResolution {
    BLOCK(16, 0),
    TWO_BLOCKS(8, 1),
    FOUR_BLOCKS(4, 2),
    HALF_CHUNK(2, 3),
    CHUNK(1, 4);

    public final int dataPointLengthCount;
    public final int dataPointWidth;
    public final byte detailLevel;
    public final int[] startX;
    public final int[] startZ;
    public final int[] endX;
    public final int[] endZ;
    private static EDhApiMaxHorizontalResolution[][] lowerDetailArrays;

    private EDhApiMaxHorizontalResolution(int newLengthCount, int newDetailLevel) {
        this.detailLevel = (byte)newDetailLevel;
        this.dataPointLengthCount = newLengthCount;
        this.dataPointWidth = 16 / this.dataPointLengthCount;
        this.startX = new int[this.dataPointLengthCount * this.dataPointLengthCount];
        this.endX = new int[this.dataPointLengthCount * this.dataPointLengthCount];
        this.startZ = new int[this.dataPointLengthCount * this.dataPointLengthCount];
        this.endZ = new int[this.dataPointLengthCount * this.dataPointLengthCount];
        int index = 0;
        for (int x = 0; x < newLengthCount; ++x) {
            for (int z = 0; z < newLengthCount; ++z) {
                this.startX[index] = x * this.dataPointWidth;
                this.startZ[index] = z * this.dataPointWidth;
                this.endX[index] = x * this.dataPointWidth + this.dataPointWidth;
                this.endZ[index] = z * this.dataPointWidth + this.dataPointWidth;
                ++index;
            }
        }
    }

    public static EDhApiMaxHorizontalResolution[] getSelfAndLowerDetails(EDhApiMaxHorizontalResolution detail) {
        if (lowerDetailArrays == null) {
            lowerDetailArrays = new EDhApiMaxHorizontalResolution[EDhApiMaxHorizontalResolution.values().length][];
            for (EDhApiMaxHorizontalResolution currentDetail : EDhApiMaxHorizontalResolution.values()) {
                ArrayList<EDhApiMaxHorizontalResolution> lowerDetails = new ArrayList<EDhApiMaxHorizontalResolution>();
                for (EDhApiMaxHorizontalResolution compareDetail : EDhApiMaxHorizontalResolution.values()) {
                    if (currentDetail.detailLevel > compareDetail.detailLevel) continue;
                    lowerDetails.add(compareDetail);
                }
                Collections.sort(lowerDetails);
                Collections.reverse(lowerDetails);
                EDhApiMaxHorizontalResolution.lowerDetailArrays[currentDetail.detailLevel] = lowerDetails.toArray(new EDhApiMaxHorizontalResolution[lowerDetails.size()]);
            }
        }
        return lowerDetailArrays[detail.detailLevel];
    }

    public static EDhApiMaxHorizontalResolution getDetailForDistance(EDhApiMaxHorizontalResolution maxDetailLevel, int distance, int maxDistance) {
        EDhApiMaxHorizontalResolution[] lowerDetails = EDhApiMaxHorizontalResolution.getSelfAndLowerDetails(maxDetailLevel);
        int distanceBetweenDetails = maxDistance / lowerDetails.length;
        int index = MathUtil.clamp(0, distance / distanceBetweenDetails, lowerDetails.length - 1);
        return lowerDetails[index];
    }
}

