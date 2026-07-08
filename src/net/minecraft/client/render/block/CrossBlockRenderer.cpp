#include <cmath>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;

namespace {
void emitCrossQuad(Tessellator& tessellator,
                   double x0,
                   double y0,
                   double z0,
                   double u0,
                   double v0,
                   double x1,
                   double y1,
                   double z1,
                   double u1,
                   double v1,
                   double x2,
                   double y2,
                   double z2,
                   double u2,
                   double v2,
                   double x3,
                   double y3,
                   double z3,
                   double u3,
                   double v3) {
    float nx = 0.0f;
    float ny = 1.0f;
    float nz = 0.0f;
    quadNormal(x0, y0, z0, x1, y1, z1, x2, y2, z2, nx, ny, nz);
    emitBlockVertex(tessellator, nx, ny, nz, x0, y0, z0, u0, v0);
    emitBlockVertex(tessellator, nx, ny, nz, x1, y1, z1, u1, v1);
    emitBlockVertex(tessellator, nx, ny, nz, x2, y2, z2, u2, v2);
    emitBlockVertex(tessellator, nx, ny, nz, x3, y3, z3, u3, v3);
}
}  // namespace

bool CrossBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
    Tessellator& tessellator = *ctx_.tess;
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    int colorMult = block.getColorMultiplier(ctx_.blockView, x, y, z);
    float red = (float) (colorMult >> 16 & 0xFF) / 255.0f;
    float green = (float) (colorMult >> 8 & 0xFF) / 255.0f;
    float blue = (float) (colorMult & 0xFF) / 255.0f;
    tessellator.color(brightness * red, brightness * green, brightness * blue);
    double dx = x;
    double dy = y;
    double dz = z;
    if (net::minecraft::block::Block::GRASS != nullptr && &block == net::minecraft::block::Block::GRASS &&
        ctx_.opts.fancyGrass) {
        std::int64_t l = (long) (x * 3129871) ^ (long) z * 116129781L ^ (long) y;
        l = l * l * 42317861L + l * 11L;
        dx += ((double) ((float) (l >> 16 & 0xFL) / 15.0f) - 0.5) * 0.5;
        dy += ((double) ((float) (l >> 20 & 0xFL) / 15.0f) - 1.0) * 0.2;
        dz += ((double) ((float) (l >> 24 & 0xFL) / 15.0f) - 0.5) * 0.5;
    }
    render(block, ctx_.blockView->getBlockMeta(x, y, z), dx, dy, dz, x, y, z);
    return true;
}

void CrossBlockRenderer::render(net::minecraft::block::Block& block, int metadata, double x, double y, double z) {
    render(block,
           metadata,
           x,
           y,
           z,
           static_cast<int>(std::floor(x)),
           static_cast<int>(std::floor(y)),
           static_cast<int>(std::floor(z)));
}

void CrossBlockRenderer::render(net::minecraft::block::Block& block,
                                int metadata,
                                double x,
                                double y,
                                double z,
                                int lightX,
                                int lightY,
                                int lightZ) {
    (void) lightX;
    (void) lightY;
    (void) lightZ;
    Tessellator& tessellator = *ctx_.tess;
    const int tex = ctx_.resolveTexture(0, block.getTexture(0, metadata));
    const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(tex);
    const double uMin = uv.uMin;
    const double uMax = uv.uMax;
    const double vMin = uv.vMin;
    const double vMax = uv.vMax;
    double x0 = x + 0.5 - (double) 0.45f;
    double x1 = x + 0.5 + (double) 0.45f;
    double z0 = z + 0.5 - (double) 0.45f;
    double z1 = z + 0.5 + (double) 0.45f;
    emitCrossQuad(tessellator,
                  x0,
                  y + 1.0,
                  z0,
                  uMin,
                  vMin,
                  x0,
                  y + 0.0,
                  z0,
                  uMin,
                  vMax,
                  x1,
                  y + 0.0,
                  z1,
                  uMax,
                  vMax,
                  x1,
                  y + 1.0,
                  z1,
                  uMax,
                  vMin);
    emitCrossQuad(tessellator,
                  x1,
                  y + 1.0,
                  z1,
                  uMin,
                  vMin,
                  x1,
                  y + 0.0,
                  z1,
                  uMin,
                  vMax,
                  x0,
                  y + 0.0,
                  z0,
                  uMax,
                  vMax,
                  x0,
                  y + 1.0,
                  z0,
                  uMax,
                  vMin);
    emitCrossQuad(tessellator,
                  x0,
                  y + 1.0,
                  z1,
                  uMin,
                  vMin,
                  x0,
                  y + 0.0,
                  z1,
                  uMin,
                  vMax,
                  x1,
                  y + 0.0,
                  z0,
                  uMax,
                  vMax,
                  x1,
                  y + 1.0,
                  z0,
                  uMax,
                  vMin);
    emitCrossQuad(tessellator,
                  x1,
                  y + 1.0,
                  z0,
                  uMin,
                  vMin,
                  x1,
                  y + 0.0,
                  z0,
                  uMin,
                  vMax,
                  x0,
                  y + 0.0,
                  z1,
                  uMax,
                  vMax,
                  x0,
                  y + 1.0,
                  z1,
                  uMax,
                  vMin);
}
}  // namespace net::minecraft::client::render::block
