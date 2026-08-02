/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.render;

import com.seibel.distanthorizons.api.objects.DhApiResult;

public interface IDhApiRenderProxy {
    public DhApiResult<Boolean> clearRenderDataCache();

    public DhApiResult<Integer> getDhDepthTextureId();

    public DhApiResult<Integer> getDhColorTextureId();

    public void setDeferTransparentRendering(boolean var1);

    public boolean getDeferTransparentRendering();

    public float getNearClipPlaneDistanceInBlocks(float var1);
}

