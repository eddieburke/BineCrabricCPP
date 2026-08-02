/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiDebugRendering;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiDebuggingConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<EDhApiDebugRendering> debugRendering();

    public IDhApiConfigValue<Boolean> debugKeybindings();

    public IDhApiConfigValue<Boolean> renderWireframe();

    public IDhApiConfigValue<Boolean> lodOnlyMode();

    public IDhApiConfigValue<Boolean> debugWireframeRendering();
}

