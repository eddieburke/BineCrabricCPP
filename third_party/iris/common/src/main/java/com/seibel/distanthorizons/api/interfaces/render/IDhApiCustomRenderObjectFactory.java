/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.render;

import com.seibel.distanthorizons.api.interfaces.render.IDhApiRenderableBoxGroup;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3d;
import com.seibel.distanthorizons.api.objects.render.DhApiRenderableBox;
import java.util.List;

public interface IDhApiCustomRenderObjectFactory {
    public IDhApiRenderableBoxGroup createForSingleBox(String var1, DhApiRenderableBox var2) throws IllegalArgumentException;

    public IDhApiRenderableBoxGroup createRelativePositionedGroup(String var1, DhApiVec3d var2, List<DhApiRenderableBox> var3);

    public IDhApiRenderableBoxGroup createAbsolutePositionedGroup(String var1, List<DhApiRenderableBox> var2);
}

