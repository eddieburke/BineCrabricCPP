/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.rendering;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.objects.math.DhApiMat4f;

public interface IDhApiCullingFrustum
extends IDhApiOverrideable {
    public void update(int var1, int var2, DhApiMat4f var3);

    public boolean intersects(int var1, int var2, int var3, int var4);
}

