/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.config.EDhApiGpuUploadMethod;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiGpuBuffersConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<EDhApiGpuUploadMethod> gpuUploadMethod();

    public IDhApiConfigValue<Integer> gpuUploadPerMegabyteInMilliseconds();
}

