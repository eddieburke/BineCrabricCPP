/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiFogFalloff;
import com.seibel.distanthorizons.api.enums.rendering.EDhApiHeightFogMixMode;
import com.seibel.distanthorizons.api.enums.rendering.EDhApiHeightFogMode;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiHeightFogConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<EDhApiHeightFogMixMode> heightFogMixMode();

    public IDhApiConfigValue<EDhApiHeightFogMode> heightFogMode();

    public IDhApiConfigValue<Double> heightFogBaseHeight();

    public IDhApiConfigValue<Double> heightFogStartingHeightPercent();

    public IDhApiConfigValue<Double> heightFogEndingHeightPercent();

    public IDhApiConfigValue<Double> heightFogMinThickness();

    public IDhApiConfigValue<Double> heightFogMaxThickness();

    public IDhApiConfigValue<EDhApiFogFalloff> heightFogFalloff();

    public IDhApiConfigValue<Double> heightFogDensity();
}

