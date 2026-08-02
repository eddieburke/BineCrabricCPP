/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.data;

import com.seibel.distanthorizons.api.interfaces.data.IDhApiTerrainDataCache;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.api.objects.DhApiResult;
import com.seibel.distanthorizons.api.objects.data.DhApiRaycastResult;
import com.seibel.distanthorizons.api.objects.data.DhApiTerrainDataPoint;

public interface IDhApiTerrainDataRepo {
    default public DhApiResult<DhApiTerrainDataPoint> getSingleDataPointAtBlockPos(IDhApiLevelWrapper levelWrapper, int blockPosX, int blockPosY, int blockPosZ) {
        return this.getSingleDataPointAtBlockPos(levelWrapper, blockPosX, blockPosY, blockPosZ, null);
    }

    public DhApiResult<DhApiTerrainDataPoint> getSingleDataPointAtBlockPos(IDhApiLevelWrapper var1, int var2, int var3, int var4, IDhApiTerrainDataCache var5);

    default public DhApiResult<DhApiTerrainDataPoint[]> getColumnDataAtBlockPos(IDhApiLevelWrapper levelWrapper, int blockPosX, int blockPosZ) {
        return this.getColumnDataAtBlockPos(levelWrapper, blockPosX, blockPosZ, null);
    }

    public DhApiResult<DhApiTerrainDataPoint[]> getColumnDataAtBlockPos(IDhApiLevelWrapper var1, int var2, int var3, IDhApiTerrainDataCache var4);

    default public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtChunkPos(IDhApiLevelWrapper levelWrapper, int chunkPosX, int chunkPosZ) {
        return this.getAllTerrainDataAtChunkPos(levelWrapper, chunkPosX, chunkPosZ, null);
    }

    public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtChunkPos(IDhApiLevelWrapper var1, int var2, int var3, IDhApiTerrainDataCache var4);

    default public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtRegionPos(IDhApiLevelWrapper levelWrapper, int regionPosX, int regionPosZ) {
        return this.getAllTerrainDataAtRegionPos(levelWrapper, regionPosX, regionPosZ, null);
    }

    public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtRegionPos(IDhApiLevelWrapper var1, int var2, int var3, IDhApiTerrainDataCache var4);

    default public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtRegionPos(IDhApiLevelWrapper levelWrapper, byte detailLevel, int posX, int posZ) {
        return this.getAllTerrainDataAtDetailLevelAndPos(levelWrapper, detailLevel, posX, posZ, null);
    }

    public DhApiResult<DhApiTerrainDataPoint[][][]> getAllTerrainDataAtDetailLevelAndPos(IDhApiLevelWrapper var1, byte var2, int var3, int var4, IDhApiTerrainDataCache var5);

    default public DhApiResult<DhApiRaycastResult> raycast(IDhApiLevelWrapper levelWrapper, double rayOriginX, double rayOriginY, double rayOriginZ, float rayDirectionX, float rayDirectionY, float rayDirectionZ, int maxRayBlockLength) {
        return this.raycast(levelWrapper, rayOriginX, rayOriginY, rayOriginZ, rayDirectionX, rayDirectionY, rayDirectionZ, maxRayBlockLength, null);
    }

    public DhApiResult<DhApiRaycastResult> raycast(IDhApiLevelWrapper var1, double var2, double var4, double var6, float var8, float var9, float var10, int var11, IDhApiTerrainDataCache var12);

    public DhApiResult<Void> overwriteChunkDataAsync(IDhApiLevelWrapper var1, Object[] var2) throws ClassCastException;

    public IDhApiTerrainDataCache getSoftCache();
}

