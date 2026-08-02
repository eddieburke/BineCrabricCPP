/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiMultiThreadingConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<Integer> worldGeneratorThreads();

    public IDhApiConfigValue<Integer> fileHandlerThreads();

    public IDhApiConfigValue<Integer> lodBuilderThreads();
}

