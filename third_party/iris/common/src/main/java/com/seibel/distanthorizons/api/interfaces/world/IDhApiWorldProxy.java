/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.world;

import com.seibel.distanthorizons.api.interfaces.world.IDhApiDimensionTypeWrapper;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;

public interface IDhApiWorldProxy {
    public boolean worldLoaded();

    public IDhApiLevelWrapper getSinglePlayerLevel() throws IllegalStateException;

    public Iterable<IDhApiLevelWrapper> getAllLoadedLevelWrappers() throws IllegalStateException;

    public Iterable<IDhApiLevelWrapper> getAllLoadedLevelsForDimensionType(IDhApiDimensionTypeWrapper var1) throws IllegalStateException;

    public Iterable<IDhApiLevelWrapper> getAllLoadedLevelsWithDimensionNameLike(String var1) throws IllegalStateException;
}

