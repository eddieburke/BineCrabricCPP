/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

public enum EDhApiBlocksToAvoid {
    NONE(false),
    NON_COLLIDING(true);

    public final boolean noCollision;

    private EDhApiBlocksToAvoid(boolean noCollision) {
        this.noCollision = noCollision;
    }
}

