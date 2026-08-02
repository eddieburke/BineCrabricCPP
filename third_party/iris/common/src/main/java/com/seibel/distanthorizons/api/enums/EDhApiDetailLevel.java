/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums;

public enum EDhApiDetailLevel {
    BLOCK(0, 1),
    CHUNK(4, 16),
    REGION(9, 512);

    public final byte detailLevel;
    public final byte widthInBlocks;

    private EDhApiDetailLevel(int detailLevel, int widthInBlocks) {
        this.detailLevel = (byte)detailLevel;
        this.widthInBlocks = (byte)widthInBlocks;
    }
}

