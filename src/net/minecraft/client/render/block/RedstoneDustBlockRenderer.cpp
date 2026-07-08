#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RedstoneWireBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
bool RedstoneDustBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
    bool connectWest;
    Tessellator& tessellator = *ctx_.tess;
    int meta = ctx_.blockView->getBlockMeta(x, y, z);
    const int tex = ctx_.resolveTexture(1, block.getTexture(1, meta));
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    float power = (float) meta / 15.0f;
    float colorRed = power * 0.6f + 0.4f;
    if (meta == 0) {
        colorRed = 0.3f;
    }
    float colorGreen = power * power * 0.7f - 0.5f;
    float colorBlue = power * power * 0.6f - 0.7f;
    if (colorGreen < 0.0f) {
        colorGreen = 0.0f;
    }
    if (colorBlue < 0.0f) {
        colorBlue = 0.0f;
    }
    tessellator.color(brightness * colorRed, brightness * colorGreen, brightness * colorBlue);
    const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(tex);
    const int texU = net::minecraft::block::Block::textureAtlasU(tex);
    const int texV = net::minecraft::block::Block::textureAtlasV(tex);
    double uMin = uv.uMin;
    double uMax = uv.uMax;
    double vMin = uv.vMin;
    double vMax = uv.vMax;
    bool connectNorth =
        net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x - 1, y, z, 1) ||
        (!ctx_.blockView->shouldSuffocate(x - 1, y, z) &&
         net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x - 1, y - 1, z, -1));
    bool connectSouth =
        net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x + 1, y, z, 3) ||
        (!ctx_.blockView->shouldSuffocate(x + 1, y, z) &&
         net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x + 1, y - 1, z, -1));
    bool connectEast = net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y, z - 1, 2) ||
                       (!ctx_.blockView->shouldSuffocate(x, y, z - 1) &&
                        net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y - 1, z - 1, -1));
    connectWest = net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y, z + 1, 0) ||
                  (!ctx_.blockView->shouldSuffocate(x, y, z + 1) &&
                   net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y - 1, z + 1, -1));
    if (!ctx_.blockView->shouldSuffocate(x, y + 1, z)) {
        if (ctx_.blockView->shouldSuffocate(x - 1, y, z) &&
            net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x - 1, y + 1, z, -1)) {
            connectNorth = true;
        }
        if (ctx_.blockView->shouldSuffocate(x + 1, y, z) &&
            net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x + 1, y + 1, z, -1)) {
            connectSouth = true;
        }
        if (ctx_.blockView->shouldSuffocate(x, y, z - 1) &&
            net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y + 1, z - 1, -1)) {
            connectEast = true;
        }
        if (ctx_.blockView->shouldSuffocate(x, y, z + 1) &&
            net::minecraft::block::RedstoneWireBlock::shouldConnectTo(ctx_.blockView, x, y + 1, z + 1, -1)) {
            connectWest = true;
        }
    }
    float x0 = x + 0;
    float x1 = x + 1;
    float z0 = z + 0;
    float z1 = z + 1;
    int wireLayout = 0;
    if ((connectNorth || connectSouth) && !connectEast && !connectWest) {
        wireLayout = 1;
    }
    if ((connectEast || connectWest) && !connectSouth && !connectNorth) {
        wireLayout = 2;
    }
    if (wireLayout != 0) {
        uMin = (float) (texU + 16) / 256.0f;
        uMax = ((float) (texU + 16) + 15.99f) / 256.0f;
        vMin = (float) texV / 256.0f;
        vMax = ((float) texV + 15.99f) / 256.0f;
    }
    if (wireLayout == 0) {
        if (connectSouth || connectEast || connectWest || connectNorth) {
            if (!connectNorth) {
                x0 += 0.3125f;
                uMin += 0.01953125;
            }
            if (!connectSouth) {
                x1 -= 0.3125f;
                uMax -= 0.01953125;
            }
            if (!connectEast) {
                z0 += 0.3125f;
                vMin += 0.01953125;
            }
            if (!connectWest) {
                z1 -= 0.3125f;
                vMax -= 0.01953125;
            }
        }
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMax, vMin);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMin, vMax);
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax + 0.0625);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMax, vMin + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMin, vMax + 0.0625);
    } else if (wireLayout == 1) {
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMax, vMin);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMin, vMax);
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax + 0.0625);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMax, vMin + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMin, vMax + 0.0625);
    } else if (wireLayout == 2) {
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMin, vMax);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMax, vMin);
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, (float) y + 0.015625f, z1, uMax, vMax + 0.0625);
        tessellator.vertex(x1, (float) y + 0.015625f, z0, uMin, vMax + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z0, uMin, vMin + 0.0625);
        tessellator.vertex(x0, (float) y + 0.015625f, z1, uMax, vMin + 0.0625);
    }
    if (!ctx_.blockView->shouldSuffocate(x, y + 1, z)) {
        uMin = (float) (texU + 16) / 256.0f;
        uMax = ((float) (texU + 16) + 15.99f) / 256.0f;
        vMin = (float) texV / 256.0f;
        vMax = ((float) texV + 15.99f) / 256.0f;
        if (ctx_.blockView->shouldSuffocate(x - 1, y, z) && ctx_.blockView->getBlockId(x - 1, y + 1, z) == 55) {
            tessellator.color(brightness * colorRed, brightness * colorGreen, brightness * colorBlue);
            tessellator.vertex((float) x + 0.015625f, (float) (y + 1) + 0.021875f, z + 1, uMax, vMin);
            tessellator.vertex((float) x + 0.015625f, y + 0, z + 1, uMin, vMin);
            tessellator.vertex((float) x + 0.015625f, y + 0, z + 0, uMin, vMax);
            tessellator.vertex((float) x + 0.015625f, (float) (y + 1) + 0.021875f, z + 0, uMax, vMax);
            tessellator.color(brightness, brightness, brightness);
            tessellator.vertex((float) x + 0.015625f, (float) (y + 1) + 0.021875f, z + 1, uMax, vMin + 0.0625);
            tessellator.vertex((float) x + 0.015625f, y + 0, z + 1, uMin, vMin + 0.0625);
            tessellator.vertex((float) x + 0.015625f, y + 0, z + 0, uMin, vMax + 0.0625);
            tessellator.vertex((float) x + 0.015625f, (float) (y + 1) + 0.021875f, z + 0, uMax, vMax + 0.0625);
        }
        if (ctx_.blockView->shouldSuffocate(x + 1, y, z) && ctx_.blockView->getBlockId(x + 1, y + 1, z) == 55) {
            tessellator.color(brightness * colorRed, brightness * colorGreen, brightness * colorBlue);
            tessellator.vertex((float) (x + 1) - 0.015625f, y + 0, z + 1, uMin, vMax);
            tessellator.vertex((float) (x + 1) - 0.015625f, (float) (y + 1) + 0.021875f, z + 1, uMax, vMax);
            tessellator.vertex((float) (x + 1) - 0.015625f, (float) (y + 1) + 0.021875f, z + 0, uMax, vMin);
            tessellator.vertex((float) (x + 1) - 0.015625f, y + 0, z + 0, uMin, vMin);
            tessellator.color(brightness, brightness, brightness);
            tessellator.vertex((float) (x + 1) - 0.015625f, y + 0, z + 1, uMin, vMax + 0.0625);
            tessellator.vertex((float) (x + 1) - 0.015625f, (float) (y + 1) + 0.021875f, z + 1, uMax, vMax + 0.0625);
            tessellator.vertex((float) (x + 1) - 0.015625f, (float) (y + 1) + 0.021875f, z + 0, uMax, vMin + 0.0625);
            tessellator.vertex((float) (x + 1) - 0.015625f, y + 0, z + 0, uMin, vMin + 0.0625);
        }
        if (ctx_.blockView->shouldSuffocate(x, y, z - 1) && ctx_.blockView->getBlockId(x, y + 1, z - 1) == 55) {
            tessellator.color(brightness * colorRed, brightness * colorGreen, brightness * colorBlue);
            tessellator.vertex(x + 1, y + 0, (float) z + 0.015625f, uMin, vMax);
            tessellator.vertex(x + 1, (float) (y + 1) + 0.021875f, (float) z + 0.015625f, uMax, vMax);
            tessellator.vertex(x + 0, (float) (y + 1) + 0.021875f, (float) z + 0.015625f, uMax, vMin);
            tessellator.vertex(x + 0, y + 0, (float) z + 0.015625f, uMin, vMin);
            tessellator.color(brightness, brightness, brightness);
            tessellator.vertex(x + 1, y + 0, (float) z + 0.015625f, uMin, vMax + 0.0625);
            tessellator.vertex(x + 1, (float) (y + 1) + 0.021875f, (float) z + 0.015625f, uMax, vMax + 0.0625);
            tessellator.vertex(x + 0, (float) (y + 1) + 0.021875f, (float) z + 0.015625f, uMax, vMin + 0.0625);
            tessellator.vertex(x + 0, y + 0, (float) z + 0.015625f, uMin, vMin + 0.0625);
        }
        if (ctx_.blockView->shouldSuffocate(x, y, z + 1) && ctx_.blockView->getBlockId(x, y + 1, z + 1) == 55) {
            tessellator.color(brightness * colorRed, brightness * colorGreen, brightness * colorBlue);
            tessellator.vertex(x + 1, (float) (y + 1) + 0.021875f, (float) (z + 1) - 0.015625f, uMax, vMin);
            tessellator.vertex(x + 1, y + 0, (float) (z + 1) - 0.015625f, uMin, vMin);
            tessellator.vertex(x + 0, y + 0, (float) (z + 1) - 0.015625f, uMin, vMax);
            tessellator.vertex(x + 0, (float) (y + 1) + 0.021875f, (float) (z + 1) - 0.015625f, uMax, vMax);
            tessellator.color(brightness, brightness, brightness);
            tessellator.vertex(x + 1, (float) (y + 1) + 0.021875f, (float) (z + 1) - 0.015625f, uMax, vMin + 0.0625);
            tessellator.vertex(x + 1, y + 0, (float) (z + 1) - 0.015625f, uMin, vMin + 0.0625);
            tessellator.vertex(x + 0, y + 0, (float) (z + 1) - 0.015625f, uMin, vMax + 0.0625);
            tessellator.vertex(x + 0, (float) (y + 1) + 0.021875f, (float) (z + 1) - 0.015625f, uMax, vMax + 0.0625);
        }
    }
    return true;
}
}  // namespace net::minecraft::client::render::block
