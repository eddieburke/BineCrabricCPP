/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.render;

public class DhApiRenderableBoxGroupShading {
    public float north = 1.0f;
    public float south = 1.0f;
    public float east = 1.0f;
    public float west = 1.0f;
    public float top = 1.0f;
    public float bottom = 1.0f;

    public static DhApiRenderableBoxGroupShading getDefaultShaded() {
        DhApiRenderableBoxGroupShading shading = new DhApiRenderableBoxGroupShading();
        shading.setDefaultShaded();
        return shading;
    }

    public static DhApiRenderableBoxGroupShading getUnshaded() {
        DhApiRenderableBoxGroupShading shading = new DhApiRenderableBoxGroupShading();
        shading.setUnshaded();
        return shading;
    }

    public void setDefaultShaded() {
        this.north = 0.8f;
        this.south = 0.8f;
        this.east = 0.6f;
        this.west = 0.6f;
        this.top = 1.0f;
        this.bottom = 0.5f;
    }

    public void setUnshaded() {
        this.north = 1.0f;
        this.south = 1.0f;
        this.east = 1.0f;
        this.west = 1.0f;
        this.top = 1.0f;
        this.bottom = 1.0f;
    }
}

