/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

public enum EDhApiWorldCompressionMode {
    MERGE_SAME_BLOCKS(0),
    VISUALLY_EQUAL(1);

    public final byte value;

    private EDhApiWorldCompressionMode(int value) {
        this.value = (byte)value;
    }

    public static EDhApiWorldCompressionMode getFromValue(byte value) {
        EDhApiWorldCompressionMode[] enumList = EDhApiWorldCompressionMode.values();
        for (int i = 0; i < enumList.length; ++i) {
            if (enumList[i].value != value) continue;
            return enumList[i];
        }
        throw new IllegalArgumentException("No lossy compression mode with the value [" + value + "]");
    }
}

