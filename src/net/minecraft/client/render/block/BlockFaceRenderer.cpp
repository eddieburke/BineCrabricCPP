#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"

#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::render::block {

void BlockFaceRenderer::renderBottomFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d = ((double)n + block.minX * 16.0) / 256.0;
    double d2 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
    double d3 = ((double)n2 + block.minZ * 16.0) / 256.0;
    double d4 = ((double)n2 + block.maxZ * 16.0 - 0.01) / 256.0;
    if (block.minX < 0.0 || block.maxX > 1.0) {
        d = ((float)n + 0.0f) / 256.0f;
        d2 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        d3 = ((float)n2 + 0.0f) / 256.0f;
        d4 = ((float)n2 + 15.99f) / 256.0f;
    }
    double d5 = d2;
    double d6 = d;
    double d7 = d3;
    double d8 = d4;
    if (ctx_.bottomFaceRotation == 2) {
        d = ((double)n + block.minZ * 16.0) / 256.0;
        d3 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
        d2 = ((double)n + block.maxZ * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
        d5 = d;
        d6 = d2;
        d3 = d4;
        d4 = d7;
    } else if (ctx_.bottomFaceRotation == 1) {
        d = ((double)(n + 16) - block.maxZ * 16.0) / 256.0;
        d3 = ((double)n2 + block.minX * 16.0) / 256.0;
        d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
        d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
        d = d5;
        d2 = d6;
        d7 = d4;
        d8 = d3;
    } else if (ctx_.bottomFaceRotation == 3) {
        d = ((double)(n + 16) - block.minX * 16.0) / 256.0;
        d2 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        d3 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
    }
    double d9 = x + block.minX;
    double d10 = x + block.maxX;
    double d11 = y + block.minY;
    double d12 = z + block.minZ;
    double d13 = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d9, d11, d13, d6, d8);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d9, d11, d12, d, d3);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d10, d11, d12, d5, d7);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d10, d11, d13, d2, d4);
    } else {
        tessellator.vertex(d9, d11, d13, d6, d8);
        tessellator.vertex(d9, d11, d12, d, d3);
        tessellator.vertex(d10, d11, d12, d5, d7);
        tessellator.vertex(d10, d11, d13, d2, d4);
    }
}

void BlockFaceRenderer::renderTopFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d = ((double)n + block.minX * 16.0) / 256.0;
    double d2 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
    double d3 = ((double)n2 + block.minZ * 16.0) / 256.0;
    double d4 = ((double)n2 + block.maxZ * 16.0 - 0.01) / 256.0;
    if (block.minX < 0.0 || block.maxX > 1.0) {
        d = ((float)n + 0.0f) / 256.0f;
        d2 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        d3 = ((float)n2 + 0.0f) / 256.0f;
        d4 = ((float)n2 + 15.99f) / 256.0f;
    }
    double d5 = d2;
    double d6 = d;
    double d7 = d3;
    double d8 = d4;
    if (ctx_.topFaceRotation == 1) {
        d = ((double)n + block.minZ * 16.0) / 256.0;
        d3 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
        d2 = ((double)n + block.maxZ * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
        d5 = d;
        d6 = d2;
        d3 = d4;
        d4 = d7;
    } else if (ctx_.topFaceRotation == 2) {
        d = ((double)(n + 16) - block.maxZ * 16.0) / 256.0;
        d3 = ((double)n2 + block.minX * 16.0) / 256.0;
        d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
        d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
        d = d5;
        d2 = d6;
        d7 = d4;
        d8 = d3;
    } else if (ctx_.topFaceRotation == 3) {
        d = ((double)(n + 16) - block.minX * 16.0) / 256.0;
        d2 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        d3 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        d5 = d2;
        d6 = d;
        d7 = d3;
        d8 = d4;
    }
    double d9 = x + block.minX;
    double d10 = x + block.maxX;
    double d11 = y + block.maxY;
    double d12 = z + block.minZ;
    double d13 = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d10, d11, d13, d2, d4);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d10, d11, d12, d5, d7);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d9, d11, d12, d, d3);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d9, d11, d13, d6, d8);
    } else {
        tessellator.vertex(d10, d11, d13, d2, d4);
        tessellator.vertex(d10, d11, d12, d5, d7);
        tessellator.vertex(d9, d11, d12, d, d3);
        tessellator.vertex(d9, d11, d13, d6, d8);
    }
}

void BlockFaceRenderer::renderEastFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double d;
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d2 = ((double)n + block.minX * 16.0) / 256.0;
    double d3 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
    double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
    double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        d = d2;
        d2 = d3;
        d3 = d;
    }
    if (block.minX < 0.0 || block.maxX > 1.0) {
        d2 = ((float)n + 0.0f) / 256.0f;
        d3 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        d4 = ((float)n2 + 0.0f) / 256.0f;
        d5 = ((float)n2 + 15.99f) / 256.0f;
    }
    d = d3;
    double d6 = d2;
    double d7 = d4;
    double d8 = d5;
    if (ctx_.eastFaceRotation == 2) {
        d2 = ((double)n + block.minY * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
        d3 = ((double)n + block.maxY * 16.0) / 256.0;
        d5 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d = d2;
        d6 = d3;
        d4 = d5;
        d5 = d7;
    } else if (ctx_.eastFaceRotation == 1) {
        d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
        d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minX * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d2 = d;
        d3 = d6;
        d7 = d5;
        d8 = d4;
    } else if (ctx_.eastFaceRotation == 3) {
        d2 = ((double)(n + 16) - block.minX * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
    }
    double d9 = x + block.minX;
    double d10 = x + block.maxX;
    double d11 = y + block.minY;
    double d12 = y + block.maxY;
    double d13 = z + block.minZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d9, d12, d13, d, d7);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d10, d12, d13, d2, d4);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d10, d11, d13, d6, d8);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d9, d11, d13, d3, d5);
    } else {
        tessellator.vertex(d9, d12, d13, d, d7);
        tessellator.vertex(d10, d12, d13, d2, d4);
        tessellator.vertex(d10, d11, d13, d6, d8);
        tessellator.vertex(d9, d11, d13, d3, d5);
    }
}

void BlockFaceRenderer::renderWestFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double d;
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d2 = ((double)n + block.minX * 16.0) / 256.0;
    double d3 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
    double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
    double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        d = d2;
        d2 = d3;
        d3 = d;
    }
    if (block.minX < 0.0 || block.maxX > 1.0) {
        d2 = ((float)n + 0.0f) / 256.0f;
        d3 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        d4 = ((float)n2 + 0.0f) / 256.0f;
        d5 = ((float)n2 + 15.99f) / 256.0f;
    }
    d = d3;
    double d6 = d2;
    double d7 = d4;
    double d8 = d5;
    if (ctx_.westFaceRotation == 1) {
        d2 = ((double)n + block.minY * 16.0) / 256.0;
        d5 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
        d3 = ((double)n + block.maxY * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d = d2;
        d6 = d3;
        d4 = d5;
        d5 = d7;
    } else if (ctx_.westFaceRotation == 2) {
        d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
        d4 = ((double)n2 + block.minX * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
        d5 = ((double)n2 + block.maxX * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d2 = d;
        d3 = d6;
        d7 = d5;
        d8 = d4;
    } else if (ctx_.westFaceRotation == 3) {
        d2 = ((double)(n + 16) - block.minX * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
    }
    double d9 = x + block.minX;
    double d10 = x + block.maxX;
    double d11 = y + block.minY;
    double d12 = y + block.maxY;
    double d13 = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d9, d12, d13, d2, d4);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d9, d11, d13, d6, d8);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d10, d11, d13, d3, d5);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d10, d12, d13, d, d7);
    } else {
        tessellator.vertex(d9, d12, d13, d2, d4);
        tessellator.vertex(d9, d11, d13, d6, d8);
        tessellator.vertex(d10, d11, d13, d3, d5);
        tessellator.vertex(d10, d12, d13, d, d7);
    }
}

void BlockFaceRenderer::renderNorthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double d;
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d2 = ((double)n + block.minZ * 16.0) / 256.0;
    double d3 = ((double)n + block.maxZ * 16.0 - 0.01) / 256.0;
    double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
    double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        d = d2;
        d2 = d3;
        d3 = d;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        d2 = ((float)n + 0.0f) / 256.0f;
        d3 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        d4 = ((float)n2 + 0.0f) / 256.0f;
        d5 = ((float)n2 + 15.99f) / 256.0f;
    }
    d = d3;
    double d6 = d2;
    double d7 = d4;
    double d8 = d5;
    if (ctx_.northFaceRotation == 1) {
        d2 = ((double)n + block.minY * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.maxZ * 16.0) / 256.0;
        d3 = ((double)n + block.maxY * 16.0) / 256.0;
        d5 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d = d2;
        d6 = d3;
        d4 = d5;
        d5 = d7;
    } else if (ctx_.northFaceRotation == 2) {
        d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
        d4 = ((double)n2 + block.minZ * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
        d5 = ((double)n2 + block.maxZ * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d2 = d;
        d3 = d6;
        d7 = d5;
        d8 = d4;
    } else if (ctx_.northFaceRotation == 3) {
        d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
    }
    double d9 = x + block.minX;
    double d10 = y + block.minY;
    double d11 = y + block.maxY;
    double d12 = z + block.minZ;
    double d13 = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d9, d11, d13, d, d7);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d9, d11, d12, d2, d4);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d9, d10, d12, d6, d8);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d9, d10, d13, d3, d5);
    } else {
        tessellator.vertex(d9, d11, d13, d, d7);
        tessellator.vertex(d9, d11, d12, d2, d4);
        tessellator.vertex(d9, d10, d12, d6, d8);
        tessellator.vertex(d9, d10, d13, d3, d5);
    }
}

void BlockFaceRenderer::renderSouthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double d;
    Tessellator& tessellator = Tessellator::INSTANCE;
    if (ctx_.textureOverride >= 0) {
        texture = ctx_.textureOverride;
    }
    int n = (texture & 0xF) << 4;
    int n2 = texture & 0xF0;
    double d2 = ((double)n + block.minZ * 16.0) / 256.0;
    double d3 = ((double)n + block.maxZ * 16.0 - 0.01) / 256.0;
    double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
    double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        d = d2;
        d2 = d3;
        d3 = d;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        d2 = ((float)n + 0.0f) / 256.0f;
        d3 = ((float)n + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        d4 = ((float)n2 + 0.0f) / 256.0f;
        d5 = ((float)n2 + 15.99f) / 256.0f;
    }
    d = d3;
    double d6 = d2;
    double d7 = d4;
    double d8 = d5;
    if (ctx_.southFaceRotation == 2) {
        d2 = ((double)n + block.minY * 16.0) / 256.0;
        d4 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
        d3 = ((double)n + block.maxY * 16.0) / 256.0;
        d5 = ((double)(n2 + 16) - block.maxZ * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d = d2;
        d6 = d3;
        d4 = d5;
        d5 = d7;
    } else if (ctx_.southFaceRotation == 1) {
        d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
        d4 = ((double)n2 + block.maxZ * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minZ * 16.0) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
        d2 = d;
        d3 = d6;
        d7 = d5;
        d8 = d4;
    } else if (ctx_.southFaceRotation == 3) {
        d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
        d3 = ((double)(n + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
        d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
        d = d3;
        d6 = d2;
        d7 = d4;
        d8 = d5;
    }
    double d9 = x + block.maxX;
    double d10 = y + block.minY;
    double d11 = y + block.maxY;
    double d12 = z + block.minZ;
    double d13 = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(d9, d10, d13, d6, d8);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(d9, d10, d12, d3, d5);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(d9, d11, d12, d, d7);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(d9, d11, d13, d2, d4);
    } else {
        tessellator.vertex(d9, d10, d13, d6, d8);
        tessellator.vertex(d9, d10, d12, d3, d5);
        tessellator.vertex(d9, d11, d12, d, d7);
        tessellator.vertex(d9, d11, d13, d2, d4);
    }
}

} // namespace net::minecraft::client::render::block
