/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.block;

import com.seibel.distanthorizons.api.interfaces.IDhApiUnsafeWrapper;

public interface IDhApiBlockStateWrapper
extends IDhApiUnsafeWrapper {
    public boolean isAir();

    public boolean isSolid();

    public boolean isLiquid();

    public String getSerialString();

    public byte getMaterialId();
}

