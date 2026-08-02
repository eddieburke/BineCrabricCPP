/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiGenericRenderingConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Boolean> renderingEnabled();

    public IDhApiConfigValue<Boolean> beaconRenderingEnabled();

    public IDhApiConfigValue<Boolean> cloudRenderingEnabled();
}

