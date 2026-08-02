/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiFogFalloff;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiFarFogConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Double> farFogStartDistance();

    public IDhApiConfigValue<Double> farFogEndDistance();

    public IDhApiConfigValue<Double> farFogMinThickness();

    public IDhApiConfigValue<Double> farFogMaxThickness();

    public IDhApiConfigValue<EDhApiFogFalloff> farFogFalloff();

    public IDhApiConfigValue<Double> farFogDensity();
}

