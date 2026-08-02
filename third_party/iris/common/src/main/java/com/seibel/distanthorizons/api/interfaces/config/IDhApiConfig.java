/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config;

import com.seibel.distanthorizons.api.interfaces.config.both.IDhApiWorldGenerationConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiDebuggingConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiGpuBuffersConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiGraphicsConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiMultiThreadingConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiMultiplayerConfig;

public interface IDhApiConfig {
    public IDhApiGraphicsConfig graphics();

    public IDhApiWorldGenerationConfig worldGenerator();

    public IDhApiMultiplayerConfig multiplayer();

    public IDhApiMultiThreadingConfig multiThreading();

    public IDhApiGpuBuffersConfig gpuBuffers();

    public IDhApiDebuggingConfig debugging();
}

