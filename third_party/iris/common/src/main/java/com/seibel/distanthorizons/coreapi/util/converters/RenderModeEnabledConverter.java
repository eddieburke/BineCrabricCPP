/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util.converters;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiRendererMode;
import com.seibel.distanthorizons.coreapi.interfaces.config.IConverter;

public class RenderModeEnabledConverter
implements IConverter<EDhApiRendererMode, Boolean> {
    @Override
    public EDhApiRendererMode convertToCoreType(Boolean renderingEnabled) {
        return renderingEnabled != false ? EDhApiRendererMode.DEFAULT : EDhApiRendererMode.DISABLED;
    }

    @Override
    public Boolean convertToApiType(EDhApiRendererMode renderingMode) {
        return renderingMode == EDhApiRendererMode.DEFAULT;
    }
}

