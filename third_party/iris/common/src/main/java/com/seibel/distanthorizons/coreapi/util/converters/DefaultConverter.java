/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util.converters;

import com.seibel.distanthorizons.coreapi.interfaces.config.IConverter;

public class DefaultConverter<T>
implements IConverter<T, T> {
    @Override
    public T convertToCoreType(T apiObject) {
        return apiObject;
    }

    @Override
    public T convertToApiType(T coreObject) {
        return coreObject;
    }
}

