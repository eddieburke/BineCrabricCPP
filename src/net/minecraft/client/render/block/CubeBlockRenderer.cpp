#include "net/minecraft/client/render/block/CubeBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::render::block {

namespace option = net::minecraft::client::option;

namespace {

bool edgeAllowsVision(const net::minecraft::BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return true;
    }
    const int blockId = blockView->getBlockId(x, y, z);
    if (blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
        return true;
    }
    return net::minecraft::block::Block::BLOCKS_ALLOW_VISION[static_cast<std::size_t>(blockId)];
}

void assignAoVertexColors(const option::ResolvedRenderOptions& resolved, BlockFaceRenderState& state, float corner0,
    float corner1, float corner2, float corner3, bool applyTint, float shade, float red, float green, float blue)
{
    corner0 = option::scaleAoCorner(corner0, resolved);
    corner1 = option::scaleAoCorner(corner1, resolved);
    corner2 = option::scaleAoCorner(corner2, resolved);
    corner3 = option::scaleAoCorner(corner3, resolved);
    const float baseRed = (applyTint ? red : 1.0f) * shade;
    const float baseGreen = (applyTint ? green : 1.0f) * shade;
    const float baseBlue = (applyTint ? blue : 1.0f) * shade;
    state.colors.red[0] = baseRed * corner0;
    state.colors.green[0] = baseGreen * corner0;
    state.colors.blue[0] = baseBlue * corner0;
    state.colors.red[1] = baseRed * corner1;
    state.colors.green[1] = baseGreen * corner1;
    state.colors.blue[1] = baseBlue * corner1;
    state.colors.red[2] = baseRed * corner2;
    state.colors.green[2] = baseGreen * corner2;
    state.colors.blue[2] = baseBlue * corner2;
    state.colors.red[3] = baseRed * corner3;
    state.colors.green[3] = baseGreen * corner3;
    state.colors.blue[3] = baseBlue * corner3;
}

void multiplyAoVertexColors(BlockFaceRenderState& state, float red, float green, float blue)
{
    for (int i = 0; i < 4; ++i) {
        state.colors.red[i] *= red;
        state.colors.green[i] *= green;
        state.colors.blue[i] *= blue;
    }
}

[[nodiscard]] float boostFlatBrightness(const option::ResolvedRenderOptions& resolved, float luminance)
{
    return option::applyBrightnessBoost(luminance, resolved);
}

[[nodiscard]] bool fancyWaterSideOverlayActive(const option::ResolvedRenderOptions& resolved)
{
    return resolved.fancyWater;
}

} // namespace

bool CubeBlockRenderer::renderBlock(net::minecraft::block::Block& block, int x, int y, int z)
{
    int n = block.getColorMultiplier(ctx_.blockView, x, y, z);
    float red = (float)(n >> 16 & 0xFF) / 255.0f;
    float green = (float)(n >> 8 & 0xFF) / 255.0f;
    float blue = (float)(n & 0xFF) / 255.0f;
    if (ctx_.opts.ambientOcclusionActive) {
        return renderSmooth(block, x, y, z, red, green, blue);
    }
    return renderFlat(block, x, y, z, red, green, blue);
}

// Faithful port of BlockRenderManager.renderSmooth (beta 1.7.3).
bool CubeBlockRenderer::renderSmooth(net::minecraft::block::Block& block, int x, int y, int z, float red, float green,
    float blue)
{
    if (ctx_.blockView == nullptr) {
        return false;
    }

    int textureId = 0;
    ctx_.faceState.useAo = true;
    bool drewAnyFace = false;
    float corner0 = 0.0f;
    float corner1 = 0.0f;
    float corner2 = 0.0f;
    float corner3 = 0.0f;

    bool tintDown = true;
    bool tintUp = true;
    bool tintEast = true;
    bool tintWest = true;
    bool tintNorth = true;
    bool tintSouth = true;

    float northBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z);
    float bottomBrightness = block.getLuminance(ctx_.blockView, x, y - 1, z);
    float eastBrightness = block.getLuminance(ctx_.blockView, x, y, z - 1);
    float southBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z);
    float topBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z);
    float westBrightness = block.getLuminance(ctx_.blockView, x, y, z + 1);

    const bool topEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y + 1, z);
    const bool bottomEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y - 1, z);
    const bool southEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y, z + 1);
    const bool northEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y, z - 1);
    const bool topWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y + 1, z);
    const bool bottomWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y - 1, z);
    const bool northWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y, z - 1);
    const bool southWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y, z + 1);
    const bool topSouthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y + 1, z + 1);
    const bool topNorthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y + 1, z - 1);
    const bool bottomSouthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y - 1, z + 1);
    const bool bottomNorthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y - 1, z - 1);

    if (block.textureId == 3) {
        tintSouth = false;
        tintNorth = false;
        tintWest = false;
        tintEast = false;
        tintDown = false;
    }
    if (ctx_.textureOverride >= 0) {
        tintSouth = false;
        tintNorth = false;
        tintWest = false;
        tintEast = false;
        tintDown = false;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y - 1, z, 0)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float northBottomBrightness = block.getLuminance(ctx_.blockView, x - 1, y - 1, z);
            float eastBottomBrightness = block.getLuminance(ctx_.blockView, x, y - 1, z - 1);
            float westBottomBrightness = block.getLuminance(ctx_.blockView, x, y - 1, z + 1);
            float southBottomBrightness = block.getLuminance(ctx_.blockView, x + 1, y - 1, z);
            const float northEastBottomBrightness = bottomNorthEdgeTranslucent || bottomWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z - 1)
                : northBottomBrightness;
            const float northWestBottomBrightness = bottomSouthEdgeTranslucent || bottomWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z + 1)
                : northBottomBrightness;
            const float southEastBottomBrightness = bottomNorthEdgeTranslucent || bottomEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z - 1)
                : southBottomBrightness;
            const float southWestBottomBrightness = bottomSouthEdgeTranslucent || bottomEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z + 1)
                : southBottomBrightness;
            corner0 = (northWestBottomBrightness + northBottomBrightness + westBottomBrightness + bottomBrightness) / 4.0f;
            corner3 = (westBottomBrightness + bottomBrightness + southWestBottomBrightness + southBottomBrightness) / 4.0f;
            corner2 = (bottomBrightness + eastBottomBrightness + southBottomBrightness + southEastBottomBrightness) / 4.0f;
            corner1 = (northBottomBrightness + northEastBottomBrightness + bottomBrightness + eastBottomBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = bottomBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintDown, 0.5f, red, green, blue);
        faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y + 1, z, 1)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float northTopBrightness = block.getLuminance(ctx_.blockView, x - 1, y + 1, z);
            float southTopBrightness = block.getLuminance(ctx_.blockView, x + 1, y + 1, z);
            float eastTopBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z - 1);
            float westTopBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z + 1);
            const float northEastTopBrightness = topNorthEdgeTranslucent || topWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z - 1)
                : northTopBrightness;
            const float southEastTopBrightness = topNorthEdgeTranslucent || topEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z - 1)
                : southTopBrightness;
            const float northWestTopBrightness = topSouthEdgeTranslucent || topWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z + 1)
                : northTopBrightness;
            const float southWestTopBrightness = topSouthEdgeTranslucent || topEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z + 1)
                : southTopBrightness;
            corner3 = (northWestTopBrightness + northTopBrightness + westTopBrightness + topBrightness) / 4.0f;
            corner0 = (westTopBrightness + topBrightness + southWestTopBrightness + southTopBrightness) / 4.0f;
            corner1 = (topBrightness + eastTopBrightness + southTopBrightness + southEastTopBrightness) / 4.0f;
            corner2 = (northTopBrightness + northEastTopBrightness + topBrightness + eastTopBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = topBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintUp, 1.0f, red, green, blue);
        faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z - 1, 2)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float northEastBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z - 1);
            float eastBottomBrightness = block.getLuminance(ctx_.blockView, x, y - 1, z - 1);
            float eastTopBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z - 1);
            float southEastBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z - 1);
            const float northEastBottomBrightness = northWestEdgeTranslucent || bottomNorthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z - 1)
                : northEastBrightness;
            const float northEastTopBrightness = northWestEdgeTranslucent || topNorthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z - 1)
                : northEastBrightness;
            const float southEastBottomBrightness = northEastEdgeTranslucent || bottomNorthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z - 1)
                : southEastBrightness;
            const float southEastTopBrightness = northEastEdgeTranslucent || topNorthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z - 1)
                : southEastBrightness;
            corner0 = (northEastBrightness + northEastTopBrightness + eastBrightness + eastTopBrightness) / 4.0f;
            corner1 = (eastBrightness + eastTopBrightness + southEastBrightness + southEastTopBrightness) / 4.0f;
            corner2 = (eastBottomBrightness + eastBrightness + southEastBottomBrightness + southEastBrightness) / 4.0f;
            corner3 = (northEastBottomBrightness + northEastBrightness + eastBottomBrightness + eastBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = eastBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintEast, 0.8f, red, green, blue);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
        faces_.renderEastFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            multiplyAoVertexColors(ctx_.faceState, red, green, blue);
            faces_.renderEastFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z + 1, 3)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float northWestBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z + 1);
            float southWestBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z + 1);
            float westBottomBrightness = block.getLuminance(ctx_.blockView, x, y - 1, z + 1);
            float westTopBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z + 1);
            const float northWestBottomBrightness = southWestEdgeTranslucent || bottomSouthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z + 1)
                : northWestBrightness;
            const float northWestTopBrightness = southWestEdgeTranslucent || topSouthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z + 1)
                : northWestBrightness;
            const float southWestBottomBrightness = southEastEdgeTranslucent || bottomSouthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z + 1)
                : southWestBrightness;
            const float southWestTopBrightness = southEastEdgeTranslucent || topSouthEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z + 1)
                : southWestBrightness;
            corner0 = (northWestBrightness + northWestTopBrightness + westBrightness + westTopBrightness) / 4.0f;
            corner3 = (westBrightness + westTopBrightness + southWestBrightness + southWestTopBrightness) / 4.0f;
            corner2 = (westBottomBrightness + westBrightness + southWestBottomBrightness + southWestBrightness) / 4.0f;
            corner1 = (northWestBottomBrightness + northWestBrightness + westBottomBrightness + westBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = westBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintWest, 0.8f, red, green, blue);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
        faces_.renderWestFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            multiplyAoVertexColors(ctx_.faceState, red, green, blue);
            faces_.renderWestFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x - 1, y, z, 4)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float northBottomBrightness = block.getLuminance(ctx_.blockView, x - 1, y - 1, z);
            float northEastBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z - 1);
            float northWestBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z + 1);
            float northTopBrightness = block.getLuminance(ctx_.blockView, x - 1, y + 1, z);
            const float northEastBottomBrightness = northWestEdgeTranslucent || bottomWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z - 1)
                : northEastBrightness;
            const float northWestBottomBrightness = southWestEdgeTranslucent || bottomWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y - 1, z + 1)
                : northWestBrightness;
            const float northEastTopBrightness = northWestEdgeTranslucent || topWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z - 1)
                : northEastBrightness;
            const float northWestTopBrightness = southWestEdgeTranslucent || topWestEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x - 1, y + 1, z + 1)
                : northWestBrightness;
            corner3 = (northBottomBrightness + northWestBottomBrightness + northBrightness + northWestBrightness) / 4.0f;
            corner0 = (northBrightness + northWestBrightness + northTopBrightness + northWestTopBrightness) / 4.0f;
            corner1 = (northEastBrightness + northBrightness + northEastTopBrightness + northTopBrightness) / 4.0f;
            corner2 = (northEastBottomBrightness + northBottomBrightness + northEastBrightness + northBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = northBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintNorth, 0.6f, red, green, blue);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
        faces_.renderNorthFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            multiplyAoVertexColors(ctx_.faceState, red, green, blue);
            faces_.renderNorthFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x + 1, y, z, 5)) {
        if (ctx_.useSurroundingBrightness > 0) {
            float southBottomBrightness = block.getLuminance(ctx_.blockView, x + 1, y - 1, z);
            float southEastBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z - 1);
            float southWestBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z + 1);
            float southTopBrightness = block.getLuminance(ctx_.blockView, x + 1, y + 1, z);
            const float southEastBottomBrightness = bottomEastEdgeTranslucent || northEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z - 1)
                : southEastBrightness;
            const float southWestBottomBrightness = bottomEastEdgeTranslucent || southEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y - 1, z + 1)
                : southWestBrightness;
            const float southEastTopBrightness = topEastEdgeTranslucent || northEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z - 1)
                : southEastBrightness;
            const float southWestTopBrightness = topEastEdgeTranslucent || southEastEdgeTranslucent
                ? block.getLuminance(ctx_.blockView, x + 1, y + 1, z + 1)
                : southWestBrightness;
            corner0 = (southBottomBrightness + southWestBottomBrightness + southBrightness + southWestBrightness) / 4.0f;
            corner3 = (southBrightness + southWestBrightness + southTopBrightness + southWestTopBrightness) / 4.0f;
            corner2 = (southEastBrightness + southBrightness + southEastTopBrightness + southTopBrightness) / 4.0f;
            corner1 = (southEastBottomBrightness + southBottomBrightness + southEastBrightness + southBrightness) / 4.0f;
        } else {
            corner2 = corner3 = corner0 = corner1 = southBrightness;
        }
        assignAoVertexColors(ctx_.opts, ctx_.faceState, corner0, corner1, corner2, corner3, tintSouth, 0.6f, red, green, blue);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 5);
        faces_.renderSouthFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            multiplyAoVertexColors(ctx_.faceState, red, green, blue);
            faces_.renderSouthFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    ctx_.faceState.useAo = false;
    return drewAnyFace;
}

// Faithful port of BlockRenderManager.renderFlat (beta 1.7.3).
bool CubeBlockRenderer::renderFlat(net::minecraft::block::Block& block, int x, int y, int z, float red, float green,
    float blue)
{
    if (ctx_.blockView == nullptr) {
        return false;
    }

    ctx_.faceState.useAo = false;
    Tessellator& tessellator = *ctx_.tess;
    bool drewAnyFace = false;
    int textureId = 0;
    float brightness = 0.0f;

    const float downRedBase = 0.5f;
    const float upRedBase = 1.0f;
    const float horizRedBase = 0.8f;
    const float nsRedBase = 0.6f;

    float downRed = downRedBase;
    float downGreen = downRedBase;
    float downBlue = downRedBase;
    float upRed = upRedBase * red;
    float upGreen = upRedBase * green;
    float upBlue = upRedBase * blue;
    float horizRed = horizRedBase;
    float horizGreen = horizRedBase;
    float horizBlue = horizRedBase;
    float nsRed = nsRedBase;
    float nsGreen = nsRedBase;
    float nsBlue = nsRedBase;

    if (net::minecraft::block::Block::GRASS_BLOCK == nullptr || &block != net::minecraft::block::Block::GRASS_BLOCK) {
        downRed *= red;
        horizRed *= red;
        nsRed *= red;
        downGreen *= green;
        horizGreen *= green;
        nsGreen *= green;
        downBlue *= blue;
        horizBlue *= blue;
        nsBlue *= blue;
    }

    const float selfBrightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z));

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y - 1, z, 0)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y - 1, z));
        tessellator.color(downRed * brightness, downGreen * brightness, downBlue * brightness);
        faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y + 1, z, 1)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y + 1, z));
        if (ctx_.renderBounds.maxY != 1.0 && !block.material.isFluid()) {
            brightness = selfBrightness;
        }
        tessellator.color(upRed * brightness, upGreen * brightness, upBlue * brightness);
        faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z - 1, 2)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z - 1));
        if (ctx_.renderBounds.minZ > 0.0) {
            brightness = selfBrightness;
        }
        tessellator.color(horizRed * brightness, horizGreen * brightness, horizBlue * brightness);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
        faces_.renderEastFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            tessellator.color(horizRed * brightness * red, horizGreen * brightness * green, horizBlue * brightness * blue);
            faces_.renderEastFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z + 1, 3)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z + 1));
        if (ctx_.renderBounds.maxZ < 1.0) {
            brightness = selfBrightness;
        }
        tessellator.color(horizRed * brightness, horizGreen * brightness, horizBlue * brightness);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
        faces_.renderWestFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            tessellator.color(horizRed * brightness * red, horizGreen * brightness * green, horizBlue * brightness * blue);
            faces_.renderWestFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x - 1, y, z, 4)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x - 1, y, z));
        if (ctx_.renderBounds.minX > 0.0) {
            brightness = selfBrightness;
        }
        tessellator.color(nsRed * brightness, nsGreen * brightness, nsBlue * brightness);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
        faces_.renderNorthFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            tessellator.color(nsRed * brightness * red, nsGreen * brightness * green, nsBlue * brightness * blue);
            faces_.renderNorthFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x + 1, y, z, 5)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x + 1, y, z));
        if (ctx_.renderBounds.maxX < 1.0) {
            brightness = selfBrightness;
        }
        tessellator.color(nsRed * brightness, nsGreen * brightness, nsBlue * brightness);
        textureId = block.getTextureId(ctx_.blockView, x, y, z, 5);
        faces_.renderSouthFace(block, x, y, z, textureId);
        if (fancyWaterSideOverlayActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
            tessellator.color(nsRed * brightness * red, nsGreen * brightness * green, nsBlue * brightness * blue);
            faces_.renderSouthFace(block, x, y, z, 38);
        }
        drewAnyFace = true;
    }

    return drewAnyFace;
}

bool CubeBlockRenderer::renderCactus(net::minecraft::block::Block& block, int x, int y, int z)
{
    int n = block.getColorMultiplier(ctx_.blockView, x, y, z);
    float red = (float)(n >> 16 & 0xFF) / 255.0f;
    float green = (float)(n >> 8 & 0xFF) / 255.0f;
    float blue = (float)(n & 0xFF) / 255.0f;
    return renderCactus(block, x, y, z, red, green, blue);
}

bool CubeBlockRenderer::renderCactus(net::minecraft::block::Block& block, int x, int y, int z, float red, float green,
    float blue)
{
    ctx_.faceState.useAo = false;
    float brightness;
    Tessellator& tessellator = *ctx_.tess;
    bool drewAnyFace = false;
    const float inset = 0.0625f;
    const float selfBrightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z));

    const float downShade = 0.5f;
    const float upShade = 1.0f;
    const float horizShade = 0.8f;
    const float nsShade = 0.6f;

    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y - 1, z, 0)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y - 1, z));
        tessellator.color(downShade * red * brightness, downShade * green * brightness, downShade * blue * brightness);
        faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
        drewAnyFace = true;
    }
    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y + 1, z, 1)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y + 1, z));
        if (ctx_.renderBounds.maxY != 1.0 && !block.material.isFluid()) {
            brightness = selfBrightness;
        }
        tessellator.color(upShade * red * brightness, upShade * green * brightness, upShade * blue * brightness);
        faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
        drewAnyFace = true;
    }
    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z - 1, 2)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z - 1));
        if (ctx_.renderBounds.minZ > 0.0) {
            brightness = selfBrightness;
        }
        tessellator.color(horizShade * red * brightness, horizShade * green * brightness, horizShade * blue * brightness);
        tessellator.translate(0.0f, 0.0f, inset);
        faces_.renderEastFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 2));
        tessellator.translate(0.0f, 0.0f, -inset);
        drewAnyFace = true;
    }
    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z + 1, 3)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x, y, z + 1));
        if (ctx_.renderBounds.maxZ < 1.0) {
            brightness = selfBrightness;
        }
        tessellator.color(horizShade * red * brightness, horizShade * green * brightness, horizShade * blue * brightness);
        tessellator.translate(0.0f, 0.0f, -inset);
        faces_.renderWestFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 3));
        tessellator.translate(0.0f, 0.0f, inset);
        drewAnyFace = true;
    }
    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x - 1, y, z, 4)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x - 1, y, z));
        if (ctx_.renderBounds.minX > 0.0) {
            brightness = selfBrightness;
        }
        tessellator.color(nsShade * red * brightness, nsShade * green * brightness, nsShade * blue * brightness);
        tessellator.translate(inset, 0.0f, 0.0f);
        faces_.renderNorthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 4));
        tessellator.translate(-inset, 0.0f, 0.0f);
        drewAnyFace = true;
    }
    if (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x + 1, y, z, 5)) {
        brightness = boostFlatBrightness(ctx_.opts, block.getLuminance(ctx_.blockView, x + 1, y, z));
        if (ctx_.renderBounds.maxX < 1.0) {
            brightness = selfBrightness;
        }
        tessellator.color(nsShade * red * brightness, nsShade * green * brightness, nsShade * blue * brightness);
        tessellator.translate(-inset, 0.0f, 0.0f);
        faces_.renderSouthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 5));
        tessellator.translate(inset, 0.0f, 0.0f);
        drewAnyFace = true;
    }
    return drewAnyFace;
}

} // namespace net::minecraft::client::render::block
