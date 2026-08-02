/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.config.EDhApiBlocksToAvoid;
import com.seibel.distanthorizons.api.enums.config.EDhApiHorizontalQuality;
import com.seibel.distanthorizons.api.enums.config.EDhApiLodShading;
import com.seibel.distanthorizons.api.enums.config.EDhApiMaxHorizontalResolution;
import com.seibel.distanthorizons.api.enums.config.EDhApiVerticalQuality;
import com.seibel.distanthorizons.api.enums.rendering.EDhApiRendererMode;
import com.seibel.distanthorizons.api.enums.rendering.EDhApiTransparency;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiAmbientOcclusionConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiFogConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiGenericRenderingConfig;
import com.seibel.distanthorizons.api.interfaces.config.client.IDhApiNoiseTextureConfig;

public interface IDhApiGraphicsConfig
extends IDhApiConfigGroup {
    public IDhApiFogConfig fog();

    public IDhApiAmbientOcclusionConfig ambientOcclusion();

    public IDhApiNoiseTextureConfig noiseTexture();

    public IDhApiGenericRenderingConfig genericRendering();

    public IDhApiConfigValue<Integer> chunkRenderDistance();

    public IDhApiConfigValue<Boolean> renderingEnabled();

    public IDhApiConfigValue<EDhApiRendererMode> renderingMode();

    public IDhApiConfigValue<EDhApiMaxHorizontalResolution> maxHorizontalResolution();

    public IDhApiConfigValue<EDhApiVerticalQuality> verticalQuality();

    public IDhApiConfigValue<EDhApiHorizontalQuality> horizontalQuality();

    public IDhApiConfigValue<EDhApiTransparency> transparency();

    public IDhApiConfigValue<EDhApiBlocksToAvoid> blocksToAvoid();

    public IDhApiConfigValue<Boolean> tintWithAvoidedBlocks();

    public IDhApiConfigValue<Double> overdrawPreventionRadius();

    public IDhApiConfigValue<Double> brightnessMultiplier();

    public IDhApiConfigValue<Double> saturationMultiplier();

    public IDhApiConfigValue<Boolean> caveCullingEnabled();

    public IDhApiConfigValue<Integer> caveCullingHeight();

    public IDhApiConfigValue<Integer> earthCurvatureRatio();

    public IDhApiConfigValue<Boolean> lodOnlyMode();

    public IDhApiConfigValue<Double> lodBias();

    public IDhApiConfigValue<EDhApiLodShading> lodShading();

    public IDhApiConfigValue<Boolean> disableFrustumCulling();

    public IDhApiConfigValue<Boolean> disableShadowFrustumCulling();
}

