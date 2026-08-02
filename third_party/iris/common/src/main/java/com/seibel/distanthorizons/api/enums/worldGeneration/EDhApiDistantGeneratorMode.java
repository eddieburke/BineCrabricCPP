/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.worldGeneration;

public enum EDhApiDistantGeneratorMode {
    PRE_EXISTING_ONLY(1),
    SURFACE(4),
    FEATURES(5);

    public final byte complexity;

    private EDhApiDistantGeneratorMode(byte complexity) {
        this.complexity = complexity;
    }
}

