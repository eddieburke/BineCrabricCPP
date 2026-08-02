/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.rendering;

public enum EDhApiTransparency {
    DISABLED(false, false),
    FAKE(true, true),
    COMPLETE(true, false);

    public final boolean transparencyEnabled;
    public final boolean fakeTransparencyEnabled;

    private EDhApiTransparency(boolean transparencyEnabled, boolean fakeTransparencyEnabled) {
        this.transparencyEnabled = transparencyEnabled;
        this.fakeTransparencyEnabled = fakeTransparencyEnabled;
    }
}

