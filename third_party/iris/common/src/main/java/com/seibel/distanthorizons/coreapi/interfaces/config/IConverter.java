/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.interfaces.config;

public interface IConverter<CoreType, ApiType> {
    public CoreType convertToCoreType(ApiType var1);

    public ApiType convertToApiType(CoreType var1);
}

