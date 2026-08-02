/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.both;

import com.seibel.distanthorizons.api.enums.worldGeneration.EDhApiDistantGeneratorMode;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiWorldGenerationConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Boolean> enableDistantWorldGeneration();

    public IDhApiConfigValue<EDhApiDistantGeneratorMode> distantGeneratorMode();
}

