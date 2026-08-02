/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiAmbientOcclusionConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Boolean> enabled();

    public IDhApiConfigValue<Integer> sampleCount();

    public IDhApiConfigValue<Double> radius();

    public IDhApiConfigValue<Double> strength();

    public IDhApiConfigValue<Double> bias();

    public IDhApiConfigValue<Double> minLight();

    public IDhApiConfigValue<Integer> blurRadius();
}

