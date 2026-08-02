/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.rendering;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.interfaces.render.IDhApiRenderableBoxGroup;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3d;
import com.seibel.distanthorizons.api.objects.render.DhApiRenderableBox;
import com.seibel.distanthorizons.api.objects.render.DhApiRenderableBoxGroupShading;

public interface IDhApiGenericObjectShaderProgram
extends IDhApiOverrideable {
    public boolean overrideThisFrame();

    public int getId();

    public void free();

    public void bind(DhApiRenderParam var1);

    public void unbind();

    public void bindVertexBuffer(int var1);

    public void fillIndirectUniformData(DhApiRenderParam var1, DhApiRenderableBoxGroupShading var2, IDhApiRenderableBoxGroup var3, DhApiVec3d var4);

    public void fillSharedDirectUniformData(DhApiRenderParam var1, DhApiRenderableBoxGroupShading var2, IDhApiRenderableBoxGroup var3, DhApiVec3d var4);

    public void fillDirectUniformData(DhApiRenderParam var1, IDhApiRenderableBoxGroup var2, DhApiRenderableBox var3, DhApiVec3d var4);
}

