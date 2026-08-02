/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.worldGenerator;

import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGenerator;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.api.objects.DhApiResult;

public interface IDhApiWorldGeneratorOverrideRegister {
    public DhApiResult<Void> registerWorldGeneratorOverride(IDhApiLevelWrapper var1, IDhApiWorldGenerator var2);
}

