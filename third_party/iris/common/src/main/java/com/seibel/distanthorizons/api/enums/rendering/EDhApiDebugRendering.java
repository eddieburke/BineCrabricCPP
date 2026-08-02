/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.rendering;

public enum EDhApiDebugRendering {
    OFF,
    SHOW_DETAIL,
    SHOW_BLOCK_MATERIAL,
    SHOW_OVERLAPPING_QUADS,
    SHOW_RENDER_SOURCE_FLAG;


    public static EDhApiDebugRendering next(EDhApiDebugRendering type) {
        switch (type.ordinal()) {
            case 0: {
                return SHOW_DETAIL;
            }
            case 1: {
                return SHOW_BLOCK_MATERIAL;
            }
            case 2: {
                return SHOW_OVERLAPPING_QUADS;
            }
            case 3: {
                return SHOW_RENDER_SOURCE_FLAG;
            }
        }
        return OFF;
    }

    public static EDhApiDebugRendering previous(EDhApiDebugRendering type) {
        switch (type.ordinal()) {
            case 0: {
                return SHOW_RENDER_SOURCE_FLAG;
            }
            case 4: {
                return SHOW_OVERLAPPING_QUADS;
            }
            case 3: {
                return SHOW_DETAIL;
            }
        }
        return OFF;
    }
}

