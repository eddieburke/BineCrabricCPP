/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.override;

import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGenerator;
import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGeneratorOverrideRegister;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.api.objects.DhApiResult;
import com.seibel.distanthorizons.coreapi.DependencyInjection.WorldGeneratorInjector;

public class DhApiWorldGeneratorOverrideRegister
implements IDhApiWorldGeneratorOverrideRegister {
    public static DhApiWorldGeneratorOverrideRegister INSTANCE = new DhApiWorldGeneratorOverrideRegister();

    private DhApiWorldGeneratorOverrideRegister() {
    }

    @Override
    public DhApiResult<Void> registerWorldGeneratorOverride(IDhApiLevelWrapper levelWrapper, IDhApiWorldGenerator worldGenerator) {
        try {
            WorldGeneratorInjector.INSTANCE.bind(levelWrapper, worldGenerator);
            return DhApiResult.createSuccess();
        }
        catch (Exception e) {
            return DhApiResult.createFail(e.getMessage());
        }
    }
}

