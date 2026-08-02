/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiFogColorMode;
import com.seibel.distanthorizons.api.enums.rendering.EDhApiFogDrawMode;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiFarFogConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiHeightFogConfig;

public interface IDhApiFogConfig
extends IDhApiConfigGroup {
    public IDhApiFarFogConfig farFog();

    public IDhApiHeightFogConfig heightFog();

    public IDhApiConfigValue<EDhApiFogDrawMode> drawMode();

    public IDhApiConfigValue<EDhApiFogColorMode> color();

    public IDhApiConfigValue<Boolean> disableVanillaFog();
}

