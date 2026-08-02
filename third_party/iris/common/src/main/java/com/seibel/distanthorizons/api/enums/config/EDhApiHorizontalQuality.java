/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

public enum EDhApiHorizontalQuality {
    LOWEST(2.0, 4),
    LOW(2.0, 8),
    MEDIUM(2.0, 12),
    HIGH((double)2.2f, 16),
    EXTREME((double)2.4f, 32);

    public final double quadraticBase;
    public final int distanceUnitInBlocks;

    private EDhApiHorizontalQuality(double quadraticBase, int distanceUnitInBlocks) {
        this.quadraticBase = quadraticBase;
        this.distanceUnitInBlocks = distanceUnitInBlocks;
    }
}

