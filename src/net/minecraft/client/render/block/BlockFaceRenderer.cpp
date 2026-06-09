#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"

#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::render::block {

void BlockFaceRenderer::renderBottomFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(0, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMin = ((double)texU + block.minX * 16.0) / 256.0;
    double uMax = ((double)texU + block.maxX * 16.0 - 0.01) / 256.0;
    double vMin = ((double)texV + block.minZ * 16.0) / 256.0;
    double vMax = ((double)texV + block.maxZ * 16.0 - 0.01) / 256.0;
    if (block.minX < 0.0 || block.maxX > 1.0) {
        uMin = ((float)texU + 0.0f) / 256.0f;
        uMax = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        vMin = ((float)texV + 0.0f) / 256.0f;
        vMax = ((float)texV + 15.99f) / 256.0f;
    }
    double uCornerA = uMax;
    double uCornerB = uMin;
    double vCornerA = vMin;
    double vCornerB = vMax;
    if (ctx_.bottomFaceRotation == 2) {
        uMin = ((double)texU + block.minZ * 16.0) / 256.0;
        vMin = ((double)(texV + 16) - block.maxX * 16.0) / 256.0;
        uMax = ((double)texU + block.maxZ * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.minX * 16.0) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uCornerA = uMin;
        uCornerB = uMax;
        vMin = vMax;
        vMax = vCornerA;
    } else if (ctx_.bottomFaceRotation == 1) {
        uMin = ((double)(texU + 16) - block.maxZ * 16.0) / 256.0;
        vMin = ((double)texV + block.minX * 16.0) / 256.0;
        uMax = ((double)(texU + 16) - block.minZ * 16.0) / 256.0;
        vMax = ((double)texV + block.maxX * 16.0) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uMin = uCornerA;
        uMax = uCornerB;
        vCornerA = vMax;
        vCornerB = vMin;
    } else if (ctx_.bottomFaceRotation == 3) {
        uMin = ((double)(texU + 16) - block.minX * 16.0) / 256.0;
        uMax = ((double)(texU + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        vMin = ((double)(texV + 16) - block.minZ * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
    }
    const double xMin = x + block.minX;
    const double xMax = x + block.maxX;
    const double yCoord = y + block.minY;
    const double zMin = z + block.minZ;
    const double zMax = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xMin, yCoord, zMax, uCornerB, vCornerB);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xMin, yCoord, zMin, uMin, vMin);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xMax, yCoord, zMin, uCornerA, vCornerA);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xMax, yCoord, zMax, uMax, vMax);
    } else {
        tessellator.vertex(xMin, yCoord, zMax, uCornerB, vCornerB);
        tessellator.vertex(xMin, yCoord, zMin, uMin, vMin);
        tessellator.vertex(xMax, yCoord, zMin, uCornerA, vCornerA);
        tessellator.vertex(xMax, yCoord, zMax, uMax, vMax);
    }
}

void BlockFaceRenderer::renderTopFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(1, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMin = ((double)texU + block.minX * 16.0) / 256.0;
    double uMax = ((double)texU + block.maxX * 16.0 - 0.01) / 256.0;
    double vMin = ((double)texV + block.minZ * 16.0) / 256.0;
    double vMax = ((double)texV + block.maxZ * 16.0 - 0.01) / 256.0;
    if (block.minX < 0.0 || block.maxX > 1.0) {
        uMin = ((float)texU + 0.0f) / 256.0f;
        uMax = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        vMin = ((float)texV + 0.0f) / 256.0f;
        vMax = ((float)texV + 15.99f) / 256.0f;
    }
    double uCornerA = uMax;
    double uCornerB = uMin;
    double vCornerA = vMin;
    double vCornerB = vMax;
    if (ctx_.topFaceRotation == 1) {
        uMin = ((double)texU + block.minZ * 16.0) / 256.0;
        vMin = ((double)(texV + 16) - block.maxX * 16.0) / 256.0;
        uMax = ((double)texU + block.maxZ * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.minX * 16.0) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uCornerA = uMin;
        uCornerB = uMax;
        vMin = vMax;
        vMax = vCornerA;
    } else if (ctx_.topFaceRotation == 2) {
        uMin = ((double)(texU + 16) - block.maxZ * 16.0) / 256.0;
        vMin = ((double)texV + block.minX * 16.0) / 256.0;
        uMax = ((double)(texU + 16) - block.minZ * 16.0) / 256.0;
        vMax = ((double)texV + block.maxX * 16.0) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
        uMin = uCornerA;
        uMax = uCornerB;
        vCornerA = vMax;
        vCornerB = vMin;
    } else if (ctx_.topFaceRotation == 3) {
        uMin = ((double)(texU + 16) - block.minX * 16.0) / 256.0;
        uMax = ((double)(texU + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        vMin = ((double)(texV + 16) - block.minZ * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        uCornerA = uMax;
        uCornerB = uMin;
        vCornerA = vMin;
        vCornerB = vMax;
    }
    const double xMin = x + block.minX;
    const double xMax = x + block.maxX;
    const double yCoord = y + block.maxY;
    const double zMin = z + block.minZ;
    const double zMax = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xMax, yCoord, zMax, uMax, vMax);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xMax, yCoord, zMin, uCornerA, vCornerA);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xMin, yCoord, zMin, uMin, vMin);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xMin, yCoord, zMax, uCornerB, vCornerB);
    } else {
        tessellator.vertex(xMax, yCoord, zMax, uMax, vMax);
        tessellator.vertex(xMax, yCoord, zMin, uCornerA, vCornerA);
        tessellator.vertex(xMin, yCoord, zMin, uMin, vMin);
        tessellator.vertex(xMin, yCoord, zMax, uCornerB, vCornerB);
    }
}

void BlockFaceRenderer::renderEastFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double uMin;
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(2, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMax = ((double)texU + block.minX * 16.0) / 256.0;
    double vMin = ((double)texU + block.maxX * 16.0 - 0.01) / 256.0;
    double vMax = ((double)(texV + 16) - block.maxY * 16.0) / 256.0;
    double uCornerA = ((double)(texV + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (block.minX < 0.0 || block.maxX > 1.0) {
        uMax = ((float)texU + 0.0f) / 256.0f;
        vMin = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        vMax = ((float)texV + 0.0f) / 256.0f;
        uCornerA = ((float)texV + 15.99f) / 256.0f;
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.eastFaceRotation == 2) {
        uMax = ((double)texU + block.minY * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.minX * 16.0) / 256.0;
        vMin = ((double)texU + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)(texV + 16) - block.maxX * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.eastFaceRotation == 1) {
        uMax = ((double)(texU + 16) - block.maxY * 16.0) / 256.0;
        vMax = ((double)texV + block.maxX * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.minY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minX * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.eastFaceRotation == 3) {
        uMax = ((double)(texU + 16) - block.minX * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        vMax = ((double)texV + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minY * 16.0 - 0.01) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xMin = x + block.minX;
    const double xMax = x + block.maxX;
    const double yMin = y + block.minY;
    const double yMax = y + block.maxY;
    const double zCoord = z + block.minZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xMin, yMax, zCoord, uMin, vCornerA);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xMax, yMax, zCoord, uMax, vMax);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xMax, yMin, zCoord, uCornerB, vCornerB);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xMin, yMin, zCoord, vMin, uCornerA);
    } else {
        tessellator.vertex(xMin, yMax, zCoord, uMin, vCornerA);
        tessellator.vertex(xMax, yMax, zCoord, uMax, vMax);
        tessellator.vertex(xMax, yMin, zCoord, uCornerB, vCornerB);
        tessellator.vertex(xMin, yMin, zCoord, vMin, uCornerA);
    }
}

void BlockFaceRenderer::renderWestFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double uMin;
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(3, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMax = ((double)texU + block.minX * 16.0) / 256.0;
    double vMin = ((double)texU + block.maxX * 16.0 - 0.01) / 256.0;
    double vMax = ((double)(texV + 16) - block.maxY * 16.0) / 256.0;
    double uCornerA = ((double)(texV + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (block.minX < 0.0 || block.maxX > 1.0) {
        uMax = ((float)texU + 0.0f) / 256.0f;
        vMin = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        vMax = ((float)texV + 0.0f) / 256.0f;
        uCornerA = ((float)texV + 15.99f) / 256.0f;
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.westFaceRotation == 1) {
        uMax = ((double)texU + block.minY * 16.0) / 256.0;
        uCornerA = ((double)(texV + 16) - block.minX * 16.0) / 256.0;
        vMin = ((double)texU + block.maxY * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.maxX * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.westFaceRotation == 2) {
        uMax = ((double)(texU + 16) - block.maxY * 16.0) / 256.0;
        vMax = ((double)texV + block.minX * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.minY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.maxX * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.westFaceRotation == 3) {
        uMax = ((double)(texU + 16) - block.minX * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.maxX * 16.0 - 0.01) / 256.0;
        vMax = ((double)texV + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minY * 16.0 - 0.01) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xMin = x + block.minX;
    const double xMax = x + block.maxX;
    const double yMin = y + block.minY;
    const double yMax = y + block.maxY;
    const double zCoord = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xMin, yMax, zCoord, uMax, vMax);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xMin, yMin, zCoord, uCornerB, vCornerB);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xMax, yMin, zCoord, vMin, uCornerA);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xMax, yMax, zCoord, uMin, vCornerA);
    } else {
        tessellator.vertex(xMin, yMax, zCoord, uMax, vMax);
        tessellator.vertex(xMin, yMin, zCoord, uCornerB, vCornerB);
        tessellator.vertex(xMax, yMin, zCoord, vMin, uCornerA);
        tessellator.vertex(xMax, yMax, zCoord, uMin, vCornerA);
    }
}

void BlockFaceRenderer::renderNorthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double uMin;
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(4, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMax = ((double)texU + block.minZ * 16.0) / 256.0;
    double vMin = ((double)texU + block.maxZ * 16.0 - 0.01) / 256.0;
    double vMax = ((double)(texV + 16) - block.maxY * 16.0) / 256.0;
    double uCornerA = ((double)(texV + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        uMax = ((float)texU + 0.0f) / 256.0f;
        vMin = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        vMax = ((float)texV + 0.0f) / 256.0f;
        uCornerA = ((float)texV + 15.99f) / 256.0f;
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.northFaceRotation == 1) {
        uMax = ((double)texU + block.minY * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.maxZ * 16.0) / 256.0;
        vMin = ((double)texU + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)(texV + 16) - block.minZ * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.northFaceRotation == 2) {
        uMax = ((double)(texU + 16) - block.maxY * 16.0) / 256.0;
        vMax = ((double)texV + block.minZ * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.minY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.maxZ * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.northFaceRotation == 3) {
        uMax = ((double)(texU + 16) - block.minZ * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        vMax = ((double)texV + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minY * 16.0 - 0.01) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xCoord = x + block.minX;
    const double yMin = y + block.minY;
    const double yMax = y + block.maxY;
    const double zMin = z + block.minZ;
    const double zMax = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xCoord, yMax, zMax, uMin, vCornerA);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xCoord, yMax, zMin, uMax, vMax);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xCoord, yMin, zMin, uCornerB, vCornerB);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xCoord, yMin, zMax, vMin, uCornerA);
    } else {
        tessellator.vertex(xCoord, yMax, zMax, uMin, vCornerA);
        tessellator.vertex(xCoord, yMax, zMin, uMax, vMax);
        tessellator.vertex(xCoord, yMin, zMin, uCornerB, vCornerB);
        tessellator.vertex(xCoord, yMin, zMax, vMin, uCornerA);
    }
}

void BlockFaceRenderer::renderSouthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture)
{
    double uMin;
    Tessellator& tessellator = Tessellator::INSTANCE;
    texture = ctx_.resolveTexture(5, texture);
    int texU = (texture & 0xF) << 4;
    int texV = texture & 0xF0;
    double uMax = ((double)texU + block.minZ * 16.0) / 256.0;
    double vMin = ((double)texU + block.maxZ * 16.0 - 0.01) / 256.0;
    double vMax = ((double)(texV + 16) - block.maxY * 16.0) / 256.0;
    double uCornerA = ((double)(texV + 16) - block.minY * 16.0 - 0.01) / 256.0;
    if (ctx_.flipTextureHorizontally) {
        uMin = uMax;
        uMax = vMin;
        vMin = uMin;
    }
    if (block.minZ < 0.0 || block.maxZ > 1.0) {
        uMax = ((float)texU + 0.0f) / 256.0f;
        vMin = ((float)texU + 15.99f) / 256.0f;
    }
    if (block.minY < 0.0 || block.maxY > 1.0) {
        vMax = ((float)texV + 0.0f) / 256.0f;
        uCornerA = ((float)texV + 15.99f) / 256.0f;
    }
    uMin = vMin;
    double uCornerB = uMax;
    double vCornerA = vMax;
    double vCornerB = uCornerA;
    if (ctx_.southFaceRotation == 2) {
        uMax = ((double)texU + block.minY * 16.0) / 256.0;
        vMax = ((double)(texV + 16) - block.minZ * 16.0) / 256.0;
        vMin = ((double)texU + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)(texV + 16) - block.maxZ * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMin = uMax;
        uCornerB = vMin;
        vMax = uCornerA;
        uCornerA = vCornerA;
    } else if (ctx_.southFaceRotation == 1) {
        uMax = ((double)(texU + 16) - block.maxY * 16.0) / 256.0;
        vMax = ((double)texV + block.maxZ * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.minY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minZ * 16.0) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
        uMax = uMin;
        vMin = uCornerB;
        vCornerA = uCornerA;
        vCornerB = vMax;
    } else if (ctx_.southFaceRotation == 3) {
        uMax = ((double)(texU + 16) - block.minZ * 16.0) / 256.0;
        vMin = ((double)(texU + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
        vMax = ((double)texV + block.maxY * 16.0) / 256.0;
        uCornerA = ((double)texV + block.minY * 16.0 - 0.01) / 256.0;
        uMin = vMin;
        uCornerB = uMax;
        vCornerA = vMax;
        vCornerB = uCornerA;
    }
    const double xCoord = x + block.maxX;
    const double yMin = y + block.minY;
    const double yMax = y + block.maxY;
    const double zMin = z + block.minZ;
    const double zMax = z + block.maxZ;
    if (ctx_.faceState.useAo) {
        tessellator.color(ctx_.faceState.colors.red[0], ctx_.faceState.colors.green[0], ctx_.faceState.colors.blue[0]);
        tessellator.vertex(xCoord, yMin, zMax, uCornerB, vCornerB);
        tessellator.color(ctx_.faceState.colors.red[1], ctx_.faceState.colors.green[1], ctx_.faceState.colors.blue[1]);
        tessellator.vertex(xCoord, yMin, zMin, vMin, uCornerA);
        tessellator.color(ctx_.faceState.colors.red[2], ctx_.faceState.colors.green[2], ctx_.faceState.colors.blue[2]);
        tessellator.vertex(xCoord, yMax, zMin, uMin, vCornerA);
        tessellator.color(ctx_.faceState.colors.red[3], ctx_.faceState.colors.green[3], ctx_.faceState.colors.blue[3]);
        tessellator.vertex(xCoord, yMax, zMax, uMax, vMax);
    } else {
        tessellator.vertex(xCoord, yMin, zMax, uCornerB, vCornerB);
        tessellator.vertex(xCoord, yMin, zMin, vMin, uCornerA);
        tessellator.vertex(xCoord, yMax, zMin, uMin, vCornerA);
        tessellator.vertex(xCoord, yMax, zMax, uMax, vMax);
    }
}

} // namespace net::minecraft::client::render::block
