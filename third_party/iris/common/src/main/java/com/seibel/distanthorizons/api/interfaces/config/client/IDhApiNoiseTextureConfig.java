/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiNoiseTextureConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Boolean> noiseEnabled();

    public IDhApiConfigValue<Integer> noiseSteps();

    public IDhApiConfigValue<Double> noiseIntensity();

    public IDhApiConfigValue<Integer> noiseDropoff();
}

