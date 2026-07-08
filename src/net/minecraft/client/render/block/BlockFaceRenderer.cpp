#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/mod/ModTexture.hpp"

namespace net::minecraft::client::render::block {
namespace {
void beginFace(BlockRenderContext& ctx, int side, int texture, Tessellator*& tessellator) {
    texture = ctx.resolveTexture(side, texture);
    ctx.bindTextureFor(texture);
    tessellator = &ctx.activeTess(texture);
}

void emitVertex(Tessellator& tessellator,
                const BlockFaceRenderState& state,
                int corner,
                float nx,
                float ny,
                float nz,
                double x,
                double y,
                double z,
                double u,
                double v) {
    (void) state;
    (void) corner;
    emitBlockVertex(tessellator, nx, ny, nz, x, y, z, u, v);
}
}  // namespace

void BlockFaceRenderer::renderBottomFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 0, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMin = (static_cast<double>(texU) + ctx_.renderBounds.minX * 16.0) * inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
    double vMin = (static_cast<double>(texV) + ctx_.renderBounds.minZ * 16.0) * inv;
    double vMax = (static_cast<double>(texV) + ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
    if (ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
        uMin = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        uMax = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
        vMin = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        vMax = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    double uCornerA = uMax;
    double uCornerB = uMin;
    double vCornerA = vMin;
    double vCornerB = vMax;
    if (ctx_.bottomFaceRotation == 2) {
        uMin = (static_cast<double>(texU) + ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxX * 16.0) * inv;
        uMax = (static_cast<double>(texU) + ctx_.renderBounds.maxZ * 16.0) * inv;
        vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uCornerA = uMin;
        uCornerB = uMax;
        vMin = vMax;
        vMax = vCornerA;
    } else if (ctx_.bottomFaceRotation == 1) {
        uMin = (static_cast<double>(texU + 16) - ctx_.renderBounds.maxZ * 16.0) * inv;
        vMin = (static_cast<double>(texV) + ctx_.renderBounds.minX * 16.0) * inv;
        uMax = (static_cast<double>(texU + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMax = (static_cast<double>(texV) + ctx_.renderBounds.maxX * 16.0) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uMin = uCornerA;
        uMax = uCornerB;
        vCornerA = vMax;
        vCornerB = vMin;
    } else if (ctx_.bottomFaceRotation == 3) {
        uMin = (static_cast<double>(texU + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        uMax = (static_cast<double>(texU + 16) - ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
        vMin = (static_cast<double>(texV + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
    }
    const double xMin = x + ctx_.renderBounds.minX;
    const double xMax = x + ctx_.renderBounds.maxX;
    const double yCoord = y + ctx_.renderBounds.minY;
    const double zMin = z + ctx_.renderBounds.minZ;
    const double zMax = z + ctx_.renderBounds.maxZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
    }
}

void BlockFaceRenderer::renderTopFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 1, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMin = (static_cast<double>(texU) + ctx_.renderBounds.minX * 16.0) * inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
    double vMin = (static_cast<double>(texV) + ctx_.renderBounds.minZ * 16.0) * inv;
    double vMax = (static_cast<double>(texV) + ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
    if (ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
        uMin = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        uMax = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
        vMin = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        vMax = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    double uCornerA = uMax;
    double uCornerB = uMin;
    double vCornerA = vMin;
    double vCornerB = vMax;
    if (ctx_.topFaceRotation == 1) {
        uMin = ((double) texU + ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = ((double) (texV + 16) - ctx_.renderBounds.maxX * 16.0) * inv;
        uMax = ((double) texU + ctx_.renderBounds.maxZ * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uCornerA = uMin;
        uCornerB = uMax;
        vMin = vMax;
        vMax = vCornerA;
    } else if (ctx_.topFaceRotation == 2) {
        uMin = ((double) (texU + 16) - ctx_.renderBounds.maxZ * 16.0) * inv;
        vMin = ((double) texV + ctx_.renderBounds.minX * 16.0) * inv;
        uMax = ((double) (texU + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxX * 16.0) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uMin = uCornerA;
        uMax = uCornerB;
        vCornerA = vMax;
        vCornerB = vMin;
    } else if (ctx_.topFaceRotation == 3) {
        uMin = ((double) (texU + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        uMax = ((double) (texU + 16) - ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
        vMin = ((double) (texV + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
    }
    const double xMin = x + ctx_.renderBounds.minX;
    const double xMax = x + ctx_.renderBounds.maxX;
    const double yCoord = y + ctx_.renderBounds.maxY;
    const double zMin = z + ctx_.renderBounds.minZ;
    const double zMax = z + ctx_.renderBounds.maxZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
    }
}

void BlockFaceRenderer::renderEastFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    double uMin;
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 2, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.minX * 16.0) * inv;
    double vMin = (static_cast<double>(texU) + ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
    double vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
    double uCornerA = (static_cast<double>(texV + 16) - ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
        uMax = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        vMin = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minY < 0.0 || ctx_.renderBounds.maxY > 1.0) {
        vMax = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        uCornerA = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.eastFaceRotation == 2) {
        uMax = ((double) texU + ctx_.renderBounds.minY * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        vMin = ((double) texU + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) (texV + 16) - ctx_.renderBounds.maxX * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.eastFaceRotation == 1) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxX * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.minY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minX * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.eastFaceRotation == 3) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xMin = x + ctx_.renderBounds.minX;
    const double xMax = x + ctx_.renderBounds.maxX;
    const double yMin = y + ctx_.renderBounds.minY;
    const double yMax = y + ctx_.renderBounds.maxY;
    const double zCoord = z + ctx_.renderBounds.minZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 0.0f, -1.0f, xMin, yMax, zCoord, uMin, vCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 0.0f, -1.0f, xMax, yMax, zCoord, uMax, vMax);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 0.0f, -1.0f, xMax, yMin, zCoord, uCornerB, vCornerB);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 0.0f, -1.0f, xMin, yMin, zCoord, vMin, uCornerA);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 0.0f, -1.0f, xMin, yMax, zCoord, uMin, vCornerA);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 0.0f, -1.0f, xMax, yMax, zCoord, uMax, vMax);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 0.0f, -1.0f, xMax, yMin, zCoord, uCornerB, vCornerB);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 0.0f, -1.0f, xMin, yMin, zCoord, vMin, uCornerA);
    }
}

void BlockFaceRenderer::renderWestFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    double uMin;
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 3, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.minX * 16.0) * inv;
    double vMin = (static_cast<double>(texU) + ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
    double vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
    double uCornerA = (static_cast<double>(texV + 16) - ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
        uMax = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        vMin = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minY < 0.0 || ctx_.renderBounds.maxY > 1.0) {
        vMax = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        uCornerA = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.westFaceRotation == 1) {
        uMax = ((double) texU + ctx_.renderBounds.minY * 16.0) * inv;
        uCornerA = ((double) (texV + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        vMin = ((double) texU + ctx_.renderBounds.maxY * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.maxX * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.westFaceRotation == 2) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
        vMax = ((double) texV + ctx_.renderBounds.minX * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.minY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.maxX * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.westFaceRotation == 3) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.minX * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.maxX * 16.0 - 0.01) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xMin = x + ctx_.renderBounds.minX;
    const double xMax = x + ctx_.renderBounds.maxX;
    const double yMin = y + ctx_.renderBounds.minY;
    const double yMax = y + ctx_.renderBounds.maxY;
    const double zCoord = z + ctx_.renderBounds.maxZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 0.0f, 1.0f, xMin, yMax, zCoord, uMax, vMax);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 0.0f, 1.0f, xMin, yMin, zCoord, uCornerB, vCornerB);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 0.0f, 1.0f, xMax, yMin, zCoord, vMin, uCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 0.0f, 1.0f, xMax, yMax, zCoord, uMin, vCornerA);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, 0.0f, 0.0f, 1.0f, xMin, yMax, zCoord, uMax, vMax);
        emitVertex(*tessellator, ctx_.faceState, 1, 0.0f, 0.0f, 1.0f, xMin, yMin, zCoord, uCornerB, vCornerB);
        emitVertex(*tessellator, ctx_.faceState, 2, 0.0f, 0.0f, 1.0f, xMax, yMin, zCoord, vMin, uCornerA);
        emitVertex(*tessellator, ctx_.faceState, 3, 0.0f, 0.0f, 1.0f, xMax, yMax, zCoord, uMin, vCornerA);
    }
}

void BlockFaceRenderer::renderNorthFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    double uMin;
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 4, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.minZ * 16.0) * inv;
    double vMin = (static_cast<double>(texU) + ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
    double vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
    double uCornerA = (static_cast<double>(texV + 16) - ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
        uMax = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        vMin = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minY < 0.0 || ctx_.renderBounds.maxY > 1.0) {
        vMax = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        uCornerA = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.northFaceRotation == 1) {
        uMax = ((double) texU + ctx_.renderBounds.minY * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.maxZ * 16.0) * inv;
        vMin = ((double) texU + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) (texV + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.northFaceRotation == 2) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
        vMax = ((double) texV + ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.minY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.maxZ * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.northFaceRotation == 3) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xCoord = x + ctx_.renderBounds.minX;
    const double yMin = y + ctx_.renderBounds.minY;
    const double yMax = y + ctx_.renderBounds.maxY;
    const double zMin = z + ctx_.renderBounds.minZ;
    const double zMax = z + ctx_.renderBounds.maxZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uMin, vCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uMax, vMax);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uCornerB, vCornerB);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, vMin, uCornerA);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uMin, vCornerA);
        emitVertex(*tessellator, ctx_.faceState, 1, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uMax, vMax);
        emitVertex(*tessellator, ctx_.faceState, 2, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uCornerB, vCornerB);
        emitVertex(*tessellator, ctx_.faceState, 3, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, vMin, uCornerA);
    }
}

void BlockFaceRenderer::renderSouthFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
    double uMin;
    Tessellator* tessellator = nullptr;
    beginFace(ctx_, 5, texture, tessellator);
    const mod::TileScale tile = mod::tileScale(texture);
    const int texU = tile.u;
    const int texV = tile.v;
    const double inv = tile.inv;
    double uMax = (static_cast<double>(texU) + ctx_.renderBounds.minZ * 16.0) * inv;
    double vMin = (static_cast<double>(texU) + ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
    double vMax = (static_cast<double>(texV + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
    double uCornerA = (static_cast<double>(texV + 16) - ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
        uMax = (static_cast<float>(texU) + 0.0f) * static_cast<float>(inv);
        vMin = (static_cast<float>(texU) + 15.99f) * static_cast<float>(inv);
    }
    if (ctx_.renderBounds.minY < 0.0 || ctx_.renderBounds.maxY > 1.0) {
        vMax = (static_cast<float>(texV) + 0.0f) * static_cast<float>(inv);
        uCornerA = (static_cast<float>(texV) + 15.99f) * static_cast<float>(inv);
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.southFaceRotation == 2) {
        uMax = ((double) texU + ctx_.renderBounds.minY * 16.0) * inv;
        vMax = ((double) (texV + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = ((double) texU + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) (texV + 16) - ctx_.renderBounds.maxZ * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.southFaceRotation == 1) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.maxY * 16.0) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxZ * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.minY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minZ * 16.0) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.southFaceRotation == 3) {
        uMax = ((double) (texU + 16) - ctx_.renderBounds.minZ * 16.0) * inv;
        vMin = ((double) (texU + 16) - ctx_.renderBounds.maxZ * 16.0 - 0.01) * inv;
        vMax = ((double) texV + ctx_.renderBounds.maxY * 16.0) * inv;
        uCornerA = ((double) texV + ctx_.renderBounds.minY * 16.0 - 0.01) * inv;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xCoord = x + ctx_.renderBounds.maxX;
    const double yMin = y + ctx_.renderBounds.minY;
    const double yMax = y + ctx_.renderBounds.maxY;
    const double zMin = z + ctx_.renderBounds.minZ;
    const double zMax = z + ctx_.renderBounds.maxZ;
    if (ctx_.faceState.useAo) {
        (*tessellator)
            .color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        emitVertex(*tessellator, ctx_.faceState, 0, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uCornerB, vCornerB);
        (*tessellator)
            .color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        emitVertex(*tessellator, ctx_.faceState, 1, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, vMin, uCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        emitVertex(*tessellator, ctx_.faceState, 2, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uMin, vCornerA);
        (*tessellator)
            .color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        emitVertex(*tessellator, ctx_.faceState, 3, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uMax, vMax);
    } else {
        emitVertex(*tessellator, ctx_.faceState, 0, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uCornerB, vCornerB);
        emitVertex(*tessellator, ctx_.faceState, 1, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, vMin, uCornerA);
        emitVertex(*tessellator, ctx_.faceState, 2, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uMin, vCornerA);
        emitVertex(*tessellator, ctx_.faceState, 3, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uMax, vMax);
    }
}
}  // namespace net::minecraft::client::render::block
