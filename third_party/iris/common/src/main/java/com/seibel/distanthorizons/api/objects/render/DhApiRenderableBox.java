/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.render;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiBlockMaterial;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3d;
import java.awt.Color;

public class DhApiRenderableBox {
    public DhApiVec3d minPos;
    public DhApiVec3d maxPos;
    public Color color;
    public byte material;

    public DhApiRenderableBox(DhApiVec3d minPos, float width, Color color, EDhApiBlockMaterial material) {
        this(minPos, new DhApiVec3d(minPos.x + (double)width, minPos.y + (double)width, minPos.z + (double)width), color, material);
    }

    public DhApiRenderableBox(DhApiVec3d minPos, DhApiVec3d maxPos, Color color, EDhApiBlockMaterial material) {
        this.minPos = minPos;
        this.maxPos = maxPos;
        this.color = color;
        this.material = material.index;
    }
}

