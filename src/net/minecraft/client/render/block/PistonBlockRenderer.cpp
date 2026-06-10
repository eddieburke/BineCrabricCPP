#include "net/minecraft/client/render/block/PistonBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/block/PistonHeadBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

void PistonBlockRenderer::renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z)
{

        ctx_.skipFaceCulling = true;
        renderPiston(block, x, y, z, true);
        ctx_.skipFaceCulling = false;
    
}

bool PistonBlockRenderer::renderPiston(net::minecraft::block::Block& block, int x, int y, int z, bool extended)
{

        int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
        bool isExtended = extended || (blockMeta & 8) != 0;
        int facing = net::minecraft::block::PistonBlock::getFacing(blockMeta);
        if (isExtended) {
            switch (facing) {
                case 0: {
                    ctx_.eastFaceRotation = 3;
                    ctx_.westFaceRotation = 3;
                    ctx_.southFaceRotation = 3;
                    ctx_.northFaceRotation = 3;
                    ctx_.setRenderBounds(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 1: {
                    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
                    break;
                }
                case 2: {
                    ctx_.southFaceRotation = 1;
                    ctx_.northFaceRotation = 2;
                    ctx_.setRenderBounds(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 3: {
                    ctx_.southFaceRotation = 2;
                    ctx_.northFaceRotation = 1;
                    ctx_.topFaceRotation = 3;
                    ctx_.bottomFaceRotation = 3;
                    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f);
                    break;
                }
                case 4: {
                    ctx_.eastFaceRotation = 1;
                    ctx_.westFaceRotation = 2;
                    ctx_.topFaceRotation = 2;
                    ctx_.bottomFaceRotation = 1;
                    ctx_.setRenderBounds(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 5: {
                    ctx_.eastFaceRotation = 2;
                    ctx_.westFaceRotation = 1;
                    ctx_.topFaceRotation = 1;
                    ctx_.bottomFaceRotation = 2;
                    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);
                }
            }
            cube_.renderBlock(block, x, y, z);
            ctx_.eastFaceRotation = 0;
            ctx_.westFaceRotation = 0;
            ctx_.southFaceRotation = 0;
            ctx_.northFaceRotation = 0;
            ctx_.topFaceRotation = 0;
            ctx_.bottomFaceRotation = 0;
            ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        } else {
            switch (facing) {
                case 0: {
                    ctx_.eastFaceRotation = 3;
                    ctx_.westFaceRotation = 3;
                    ctx_.southFaceRotation = 3;
                    ctx_.northFaceRotation = 3;
                    break;
                }
                case 1: {
                    break;
                }
                case 2: {
                    ctx_.southFaceRotation = 1;
                    ctx_.northFaceRotation = 2;
                    break;
                }
                case 3: {
                    ctx_.southFaceRotation = 2;
                    ctx_.northFaceRotation = 1;
                    ctx_.topFaceRotation = 3;
                    ctx_.bottomFaceRotation = 3;
                    break;
                }
                case 4: {
                    ctx_.eastFaceRotation = 1;
                    ctx_.westFaceRotation = 2;
                    ctx_.topFaceRotation = 2;
                    ctx_.bottomFaceRotation = 1;
                    break;
                }
                case 5: {
                    ctx_.eastFaceRotation = 2;
                    ctx_.westFaceRotation = 1;
                    ctx_.topFaceRotation = 1;
                    ctx_.bottomFaceRotation = 2;
                }
            }
            cube_.renderBlock(block, x, y, z);
            ctx_.eastFaceRotation = 0;
            ctx_.westFaceRotation = 0;
            ctx_.southFaceRotation = 0;
            ctx_.northFaceRotation = 0;
            ctx_.topFaceRotation = 0;
            ctx_.bottomFaceRotation = 0;
        }
        return true;
    
}

void PistonBlockRenderer::renderExtensionRod(int axis, double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double textureScrollU)
{
        const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainStripUv(
            net::minecraft::block::PistonConstants::TEXTURE_EXTENSION, textureScrollU);
        Tessellator& tessellator = *ctx_.tess;
        tessellator.color(brightness, brightness, brightness);
        switch (axis) {
        case 0:
            tessellator.vertex(x1, y2, z1, uv.uMax, uv.vMin);
            tessellator.vertex(x1, y1, z1, uv.uMin, uv.vMin);
            tessellator.vertex(x2, y1, z2, uv.uMin, uv.vMax);
            tessellator.vertex(x2, y2, z2, uv.uMax, uv.vMax);
            break;
        case 1:
            tessellator.vertex(x1, y1, z2, uv.uMax, uv.vMin);
            tessellator.vertex(x1, y1, z1, uv.uMin, uv.vMin);
            tessellator.vertex(x2, y2, z1, uv.uMin, uv.vMax);
            tessellator.vertex(x2, y2, z2, uv.uMax, uv.vMax);
            break;
        default:
            tessellator.vertex(x2, y1, z1, uv.uMax, uv.vMin);
            tessellator.vertex(x1, y1, z1, uv.uMin, uv.vMin);
            tessellator.vertex(x1, y2, z2, uv.uMin, uv.vMax);
            tessellator.vertex(x2, y2, z2, uv.uMax, uv.vMax);
            break;
        }
    
}

void PistonBlockRenderer::renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway)
{

        ctx_.skipFaceCulling = true;
        renderPistonHead(block, x, y, z, extendedHalfway);
        ctx_.skipFaceCulling = false;
    
}

bool PistonBlockRenderer::renderPistonHead(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway)
{

        const int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
        const int facing = net::minecraft::block::PistonHeadBlock::getFacing(blockMeta);
        const float brightness = block.getLuminance(ctx_.blockView, x, y, z);
        const float extensionDepth = extendedHalfway ? 1.0f : 0.5f;
        const double textureScrollU = extendedHalfway ? 16.0 : 8.0;
        switch (facing) {
            case 0: {
                ctx_.eastFaceRotation = 3;
                ctx_.westFaceRotation = 3;
                ctx_.southFaceRotation = 3;
                ctx_.northFaceRotation = 3;
                ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(0, (float)x + 0.375f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + extensionDepth, (float)z + 0.625f, (float)z + 0.625f, brightness * 0.8f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.625f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + extensionDepth, (float)z + 0.375f, (float)z + 0.375f, brightness * 0.8f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.375f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + extensionDepth, (float)z + 0.375f, (float)z + 0.625f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.625f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + extensionDepth, (float)z + 0.625f, (float)z + 0.375f, brightness * 0.6f, textureScrollU);
                break;
            }
            case 1: {
                ctx_.setRenderBounds(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(0, (float)x + 0.375f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - extensionDepth, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.625f, brightness * 0.8f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.625f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - extensionDepth, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.375f, brightness * 0.8f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.375f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - extensionDepth, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.625f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(0, (float)x + 0.625f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - extensionDepth, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.375f, brightness * 0.6f, textureScrollU);
                break;
            }
            case 2: {
                ctx_.southFaceRotation = 1;
                ctx_.northFaceRotation = 2;
                ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(1, (float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + extensionDepth, brightness * 0.6f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + extensionDepth, brightness * 0.6f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + extensionDepth, brightness * 0.5f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + extensionDepth, brightness, textureScrollU);
                break;
            }
            case 3: {
                ctx_.southFaceRotation = 2;
                ctx_.northFaceRotation = 1;
                ctx_.topFaceRotation = 3;
                ctx_.bottomFaceRotation = 3;
                ctx_.setRenderBounds(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(1, (float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - extensionDepth, (float)z - 0.25f + 1.0f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - extensionDepth, (float)z - 0.25f + 1.0f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - extensionDepth, (float)z - 0.25f + 1.0f, brightness * 0.5f, textureScrollU);
                renderExtensionRod(1, (float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - extensionDepth, (float)z - 0.25f + 1.0f, brightness, textureScrollU);
                break;
            }
            case 4: {
                ctx_.eastFaceRotation = 1;
                ctx_.westFaceRotation = 2;
                ctx_.topFaceRotation = 2;
                ctx_.bottomFaceRotation = 1;
                ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(2, (float)x + 0.25f, (float)x + 0.25f + extensionDepth, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, brightness * 0.5f, textureScrollU);
                renderExtensionRod(2, (float)x + 0.25f, (float)x + 0.25f + extensionDepth, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, brightness, textureScrollU);
                renderExtensionRod(2, (float)x + 0.25f, (float)x + 0.25f + extensionDepth, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(2, (float)x + 0.25f, (float)x + 0.25f + extensionDepth, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, brightness * 0.6f, textureScrollU);
                break;
            }
            case 5: {
                ctx_.eastFaceRotation = 2;
                ctx_.westFaceRotation = 1;
                ctx_.topFaceRotation = 1;
                ctx_.bottomFaceRotation = 2;
                ctx_.setRenderBounds(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderExtensionRod(2, (float)x - 0.25f + 1.0f - extensionDepth, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, brightness * 0.5f, textureScrollU);
                renderExtensionRod(2, (float)x - 0.25f + 1.0f - extensionDepth, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, brightness, textureScrollU);
                renderExtensionRod(2, (float)x - 0.25f + 1.0f - extensionDepth, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, brightness * 0.6f, textureScrollU);
                renderExtensionRod(2, (float)x - 0.25f + 1.0f - extensionDepth, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, brightness * 0.6f, textureScrollU);
            }
        }
        ctx_.eastFaceRotation = 0;
        ctx_.westFaceRotation = 0;
        ctx_.southFaceRotation = 0;
        ctx_.northFaceRotation = 0;
        ctx_.topFaceRotation = 0;
        ctx_.bottomFaceRotation = 0;
        ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return true;
    
}

} // namespace net::minecraft::client::render::block
