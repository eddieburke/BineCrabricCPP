/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.rendering;

public enum EDhApiRendererMode {
    DEFAULT,
    DEBUG,
    DISABLED;


    public static EDhApiRendererMode next(EDhApiRendererMode type) {
        switch (type.ordinal()) {
            case 0: {
                return DEBUG;
            }
            case 1: {
                return DISABLED;
            }
        }
        return DEFAULT;
    }

    public static EDhApiRendererMode previous(EDhApiRendererMode type) {
        switch (type.ordinal()) {
            case 0: {
                return DISABLED;
            }
            case 1: {
                return DEFAULT;
            }
        }
        return DEBUG;
    }
}

