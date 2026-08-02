/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.sharedParameterObjects;

import com.seibel.distanthorizons.api.enums.rendering.EDhApiRenderPass;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.objects.math.DhApiMat4f;

public class DhApiRenderParam
implements IDhApiEventParam {
    public final EDhApiRenderPass renderPass;
    public final float partialTicks;
    public final float nearClipPlane;
    public final float farClipPlane;
    public final DhApiMat4f mcProjectionMatrix;
    public final DhApiMat4f mcModelViewMatrix;
    public final DhApiMat4f dhProjectionMatrix;
    public final DhApiMat4f dhModelViewMatrix;
    public final int worldYOffset;

    public DhApiRenderParam(DhApiRenderParam parent) {
        this(parent.renderPass, parent.partialTicks, parent.nearClipPlane, parent.farClipPlane, parent.mcProjectionMatrix.copy(), parent.mcModelViewMatrix.copy(), parent.dhProjectionMatrix.copy(), parent.dhModelViewMatrix.copy(), parent.worldYOffset);
    }

    public DhApiRenderParam(EDhApiRenderPass renderPass, float newPartialTicks, float nearClipPlane, float farClipPlane, DhApiMat4f newMcProjectionMatrix, DhApiMat4f newMcModelViewMatrix, DhApiMat4f newDhProjectionMatrix, DhApiMat4f newDhModelViewMatrix, int worldYOffset) {
        this.renderPass = renderPass;
        this.partialTicks = newPartialTicks;
        this.farClipPlane = farClipPlane;
        this.nearClipPlane = nearClipPlane;
        this.mcProjectionMatrix = newMcProjectionMatrix;
        this.mcModelViewMatrix = newMcModelViewMatrix;
        this.dhProjectionMatrix = newDhProjectionMatrix;
        this.dhModelViewMatrix = newDhModelViewMatrix;
        this.worldYOffset = worldYOffset;
    }

    @Override
    public DhApiRenderParam copy() {
        return new DhApiRenderParam(this);
    }
}

