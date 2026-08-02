/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.rendering;

public enum EDhApiBlockMaterial {
    UNKNOWN(0),
    LEAVES(1),
    STONE(2),
    WOOD(3),
    METAL(4),
    DIRT(5),
    LAVA(6),
    DEEPSLATE(7),
    SNOW(8),
    SAND(9),
    TERRACOTTA(10),
    NETHER_STONE(11),
    WATER(12),
    GRASS(13),
    AIR(14),
    ILLUMINATED(15);

    public final byte index;

    private EDhApiBlockMaterial(int index) {
        this.index = (byte)index;
    }

    public static EDhApiBlockMaterial getFromIndex(int index) {
        for (EDhApiBlockMaterial material : EDhApiBlockMaterial.values()) {
            if (material.index != index) continue;
            return material;
        }
        return UNKNOWN;
    }
}

