/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

public enum EDhApiDataCompressionMode {
    UNCOMPRESSED(0),
    LZ4(1),
    LZMA2(3);

    public final byte value;

    private EDhApiDataCompressionMode(int value) {
        this.value = (byte)value;
    }

    public static EDhApiDataCompressionMode getFromValue(byte value) throws IllegalArgumentException {
        EDhApiDataCompressionMode[] enumList = EDhApiDataCompressionMode.values();
        for (int i = 0; i < enumList.length; ++i) {
            if (enumList[i].value != value) continue;
            return enumList[i];
        }
        throw new IllegalArgumentException("No compression mode with the value [" + value + "]");
    }
}

