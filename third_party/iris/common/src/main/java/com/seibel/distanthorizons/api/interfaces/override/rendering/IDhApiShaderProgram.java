/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.rendering;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3f;

public interface IDhApiShaderProgram
extends IDhApiOverrideable {
    public boolean overrideThisFrame();

    public int getId();

    public void free();

    public void bind();

    public void unbind();

    public void fillUniformData(DhApiRenderParam var1);

    public void setModelOffsetPos(DhApiVec3f var1);

    public void bindVertexBuffer(int var1);
}

