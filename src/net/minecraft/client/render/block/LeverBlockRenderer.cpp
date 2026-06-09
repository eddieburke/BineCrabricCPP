#include "net/minecraft/client/render/block/LeverBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/util/math/Vec3dClient.hpp"
#include "net/minecraft/world/BlockView.hpp"

#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace net::minecraft::client::render::block {
bool LeverBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{

        int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
        int facing = blockMeta & 7;
        bool isPowered = (blockMeta & 8) > 0;
        Tessellator& tessellator = render::INSTANCE;
        const bool hadTextureOverride = ctx_.textureOverride >= 0;
        if (!hadTextureOverride) {
            ctx_.textureOverride = net::minecraft::block::Block::BLOCKS[4]->textureId;
        }
        constexpr float halfDepth = 0.25f;
        constexpr float halfWidth = 0.1875f;
        constexpr float handleHeight = 0.1875f;
        if (facing == 5) {
            block.setBoundingBox(0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, handleHeight, 0.5f + halfDepth);
        } else if (facing == 6) {
            block.setBoundingBox(0.5f - halfDepth, 0.0f, 0.5f - halfWidth, 0.5f + halfDepth, handleHeight, 0.5f + halfWidth);
        } else if (facing == 4) {
            block.setBoundingBox(0.5f - halfWidth, 0.5f - halfDepth, 1.0f - handleHeight, 0.5f + halfWidth, 0.5f + halfDepth, 1.0f);
        } else if (facing == 3) {
            block.setBoundingBox(0.5f - halfWidth, 0.5f - halfDepth, 0.0f, 0.5f + halfWidth, 0.5f + halfDepth, handleHeight);
        } else if (facing == 2) {
            block.setBoundingBox(1.0f - handleHeight, 0.5f - halfDepth, 0.5f - halfWidth, 1.0f, 0.5f + halfDepth, 0.5f + halfWidth);
        } else if (facing == 1) {
            block.setBoundingBox(0.0f, 0.5f - halfDepth, 0.5f - halfWidth, handleHeight, 0.5f + halfDepth, 0.5f + halfWidth);
        }
        cube_.renderBlock(block, x, y, z);
        if (!hadTextureOverride) {
            ctx_.textureOverride = -1;
        }
        float brightness = block.getLuminance(ctx_.blockView, x, y, z);
        if (net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            brightness = 1.0f;
        }
        tessellator.color(brightness, brightness, brightness);
        const int texture = ctx_.resolveTexture(0, block.getTexture(0));
        const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(texture);
        const int texU = net::minecraft::block::Block::textureAtlasU(texture);
        const int texV = net::minecraft::block::Block::textureAtlasV(texture);
        float uMin = static_cast<float>(uv.uMin);
        float uMax = static_cast<float>(uv.uMax);
        float vMin = static_cast<float>(uv.vMin);
        float vMax = static_cast<float>(uv.vMax);

        constexpr float stickHalfWidth = 0.0625f;
        constexpr float stickHalfDepth = 0.0625f;
        constexpr float stickHeight = 0.625f;
        std::array<net::minecraft::util::math::ClientVec3d*, 8> handleCorners {};
        handleCorners[0] = &net::minecraft::util::math::ClientVec3d::createCached(-stickHalfWidth, 0.0, -stickHalfDepth);
        handleCorners[1] = &net::minecraft::util::math::ClientVec3d::createCached(stickHalfWidth, 0.0, -stickHalfDepth);
        handleCorners[2] = &net::minecraft::util::math::ClientVec3d::createCached(stickHalfWidth, 0.0, stickHalfDepth);
        handleCorners[3] = &net::minecraft::util::math::ClientVec3d::createCached(-stickHalfWidth, 0.0, stickHalfDepth);
        handleCorners[4] = &net::minecraft::util::math::ClientVec3d::createCached(-stickHalfWidth, stickHeight, -stickHalfDepth);
        handleCorners[5] = &net::minecraft::util::math::ClientVec3d::createCached(stickHalfWidth, stickHeight, -stickHalfDepth);
        handleCorners[6] = &net::minecraft::util::math::ClientVec3d::createCached(stickHalfWidth, stickHeight, stickHalfDepth);
        handleCorners[7] = &net::minecraft::util::math::ClientVec3d::createCached(-stickHalfWidth, stickHeight, stickHalfDepth);
        for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
            net::minecraft::util::math::ClientVec3d* corner = handleCorners[static_cast<std::size_t>(cornerIndex)];
            if (isPowered) {
                corner->z -= 0.0625;
                corner->rotateX(0.69813174f);
            } else {
                corner->z += 0.0625;
                corner->rotateX(-0.69813174f);
            }
            if (facing == 6) {
                corner->rotateY(1.5707964f);
            }
            if (facing < 5) {
                corner->y -= 0.375;
                corner->rotateX(1.5707964f);
                if (facing == 4) {
                    corner->rotateY(0.0f);
                }
                if (facing == 3) {
                    corner->rotateY(static_cast<float>(M_PI));
                }
                if (facing == 2) {
                    corner->rotateY(1.5707964f);
                }
                if (facing == 1) {
                    corner->rotateY(-1.5707964f);
                }
                corner->x += static_cast<double>(x) + 0.5;
                corner->y += static_cast<double>(static_cast<float>(y) + 0.5f);
                corner->z += static_cast<double>(z) + 0.5;
                continue;
            }
            corner->x += static_cast<double>(x) + 0.5;
            corner->y += static_cast<double>(static_cast<float>(y) + 0.125f);
            corner->z += static_cast<double>(z) + 0.5;
        }
        net::minecraft::util::math::ClientVec3d* cornerA = nullptr;
        net::minecraft::util::math::ClientVec3d* cornerB = nullptr;
        net::minecraft::util::math::ClientVec3d* cornerC = nullptr;
        net::minecraft::util::math::ClientVec3d* cornerD = nullptr;
        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            if (faceIndex == 0) {
                uMin = static_cast<float>(texU + 7) / 256.0f;
                uMax = (static_cast<float>(texU + 9) - 0.01f) / 256.0f;
                vMin = static_cast<float>(texV + 6) / 256.0f;
                vMax = (static_cast<float>(texV + 8) - 0.01f) / 256.0f;
            } else if (faceIndex == 2) {
                uMin = static_cast<float>(texU + 7) / 256.0f;
                uMax = (static_cast<float>(texU + 9) - 0.01f) / 256.0f;
                vMin = static_cast<float>(texV + 6) / 256.0f;
                vMax = (static_cast<float>(texV + 16) - 0.01f) / 256.0f;
            }
            if (faceIndex == 0) {
                cornerA = handleCorners[0];
                cornerB = handleCorners[1];
                cornerC = handleCorners[2];
                cornerD = handleCorners[3];
            } else if (faceIndex == 1) {
                cornerA = handleCorners[7];
                cornerB = handleCorners[6];
                cornerC = handleCorners[5];
                cornerD = handleCorners[4];
            } else if (faceIndex == 2) {
                cornerA = handleCorners[1];
                cornerB = handleCorners[0];
                cornerC = handleCorners[4];
                cornerD = handleCorners[5];
            } else if (faceIndex == 3) {
                cornerA = handleCorners[2];
                cornerB = handleCorners[1];
                cornerC = handleCorners[5];
                cornerD = handleCorners[6];
            } else if (faceIndex == 4) {
                cornerA = handleCorners[3];
                cornerB = handleCorners[2];
                cornerC = handleCorners[6];
                cornerD = handleCorners[7];
            } else if (faceIndex == 5) {
                cornerA = handleCorners[0];
                cornerB = handleCorners[3];
                cornerC = handleCorners[7];
                cornerD = handleCorners[4];
            }
            tessellator.vertex(cornerA->x, cornerA->y, cornerA->z, uMin, vMax);
            tessellator.vertex(cornerB->x, cornerB->y, cornerB->z, uMax, vMax);
            tessellator.vertex(cornerC->x, cornerC->y, cornerC->z, uMax, vMin);
            tessellator.vertex(cornerD->x, cornerD->y, cornerD->z, uMin, vMin);
        }
        return true;
    
}


} // namespace net::minecraft::client::render::block

