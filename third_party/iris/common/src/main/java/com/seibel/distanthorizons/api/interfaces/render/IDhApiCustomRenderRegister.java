/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.render;

import com.seibel.distanthorizons.api.interfaces.render.IDhApiRenderableBoxGroup;

public interface IDhApiCustomRenderRegister {
    public void add(IDhApiRenderableBoxGroup var1) throws IllegalArgumentException;

    public IDhApiRenderableBoxGroup remove(long var1);
}

