/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.factories;

import com.seibel.distanthorizons.api.interfaces.block.IDhApiBiomeWrapper;
import com.seibel.distanthorizons.api.interfaces.block.IDhApiBlockStateWrapper;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import java.io.IOException;

public interface IDhApiWrapperFactory {
    public IDhApiBiomeWrapper getBiomeWrapper(Object[] var1, IDhApiLevelWrapper var2) throws ClassCastException;

    public IDhApiBlockStateWrapper getBlockStateWrapper(Object[] var1, IDhApiLevelWrapper var2) throws ClassCastException;

    public IDhApiBlockStateWrapper getAirBlockStateWrapper();

    public IDhApiBiomeWrapper getBiomeWrapper(String var1, IDhApiLevelWrapper var2) throws IOException, ClassCastException;

    public IDhApiBlockStateWrapper getDefaultBlockStateWrapper(String var1, IDhApiLevelWrapper var2) throws IOException, ClassCastException;
}

