/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

import com.seibel.distanthorizons.coreapi.util.MathUtil;

public enum EDhApiVerticalQuality {
    HEIGHT_MAP(new int[]{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}),
    LOW(new int[]{4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 1}),
    MEDIUM(new int[]{6, 4, 3, 3, 3, 3, 3, 2, 2, 2, 1}),
    HIGH(new int[]{16, 8, 4, 3, 3, 3, 3, 3, 3, 3, 1}),
    VERY_HIGH(new int[]{32, 16, 8, 4, 4, 3, 3, 3, 3, 3, 1}),
    EXTREME(new int[]{64, 32, 8, 4, 4, 3, 3, 3, 3, 3, 1}),
    PIXEL_ART(new int[]{512, 64, 16, 8, 4, 3, 3, 3, 3, 3, 1});

    public final int[] maxVerticalData;

    private EDhApiVerticalQuality(int[] maxVerticalData) {
        this.maxVerticalData = maxVerticalData;
    }

    public int calculateMaxVerticalData(byte dataDetail) {
        int index = MathUtil.clamp(0, dataDetail, this.maxVerticalData.length - 1);
        return this.maxVerticalData[index];
    }
}

