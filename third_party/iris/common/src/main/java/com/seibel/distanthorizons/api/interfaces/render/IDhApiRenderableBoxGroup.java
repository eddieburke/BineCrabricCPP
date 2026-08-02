/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.render;

import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3d;
import com.seibel.distanthorizons.api.objects.render.DhApiRenderableBox;
import com.seibel.distanthorizons.api.objects.render.DhApiRenderableBoxGroupShading;
import java.util.List;
import java.util.function.Consumer;

public interface IDhApiRenderableBoxGroup
extends List<DhApiRenderableBox> {
    public long getId();

    public String getResourceLocationNamespace();

    public String getResourceLocationPath();

    public void setActive(boolean var1);

    public boolean isActive();

    public void setSsaoEnabled(boolean var1);

    public boolean isSsaoEnabled();

    public void setOriginBlockPos(DhApiVec3d var1);

    public DhApiVec3d getOriginBlockPos();

    public void setPreRenderFunc(Consumer<DhApiRenderParam> var1);

    public void setPostRenderFunc(Consumer<DhApiRenderParam> var1);

    public void triggerBoxChange();

    public void setSkyLight(int var1);

    public int getSkyLight();

    public void setBlockLight(int var1);

    public int getBlockLight();

    public void setShading(DhApiRenderableBoxGroupShading var1);

    public DhApiRenderableBoxGroupShading getShading();
}

