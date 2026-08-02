/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.data;

import com.seibel.distanthorizons.api.objects.data.DhApiTerrainDataPoint;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3i;

public class DhApiRaycastResult {
    public final DhApiVec3i pos;
    public final DhApiTerrainDataPoint dataPoint;

    public DhApiRaycastResult(DhApiTerrainDataPoint dataPoint, DhApiVec3i blockPos) {
        this.dataPoint = dataPoint;
        this.pos = blockPos;
    }
}

