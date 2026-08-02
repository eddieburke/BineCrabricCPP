/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.world;

import com.seibel.distanthorizons.api.interfaces.IDhApiUnsafeWrapper;

public interface IDhApiDimensionTypeWrapper
extends IDhApiUnsafeWrapper {
    public String getDimensionName();

    public boolean hasCeiling();

    public boolean hasSkyLight();
}

