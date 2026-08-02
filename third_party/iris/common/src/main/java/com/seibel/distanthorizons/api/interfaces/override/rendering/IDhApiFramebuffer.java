/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.rendering;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;

public interface IDhApiFramebuffer
extends IDhApiOverrideable {
    public boolean overrideThisFrame();

    public void bind();

    public void addDepthAttachment(int var1, boolean var2);

    public int getId();

    public int getStatus();

    public void addColorAttachment(int var1, int var2);

    public void destroy();
}

