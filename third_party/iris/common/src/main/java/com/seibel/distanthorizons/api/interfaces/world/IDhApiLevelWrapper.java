/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.world;

import com.seibel.distanthorizons.api.enums.worldGeneration.EDhApiLevelType;
import com.seibel.distanthorizons.api.interfaces.IDhApiUnsafeWrapper;
import com.seibel.distanthorizons.api.interfaces.render.IDhApiCustomRenderRegister;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiDimensionTypeWrapper;

public interface IDhApiLevelWrapper
extends IDhApiUnsafeWrapper {
    public IDhApiDimensionTypeWrapper getDimensionType();

    public EDhApiLevelType getLevelType();

    public boolean hasCeiling();

    public boolean hasSkyLight();

    @Deprecated
    default public int getHeight() {
        return this.getMaxHeight();
    }

    public int getMaxHeight();

    default public int getMinHeight() {
        return 0;
    }

    public IDhApiCustomRenderRegister getRenderRegister();
}

