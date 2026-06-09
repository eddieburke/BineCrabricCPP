#include "net/minecraft/client/render/block/PistonBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
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

        int n = ctx_.blockView->getBlockMeta(x, y, z);
        bool bl = extended || (n & 8) != 0;
        int n2 = net::minecraft::block::PistonBlock::getFacing(n);
        if (bl) {
            switch (n2) {
                case 0: {
                    ctx_.eastFaceRotation = 3;
                    ctx_.westFaceRotation = 3;
                    ctx_.southFaceRotation = 3;
                    ctx_.northFaceRotation = 3;
                    block.setBoundingBox(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 1: {
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
                    break;
                }
                case 2: {
                    ctx_.southFaceRotation = 1;
                    ctx_.northFaceRotation = 2;
                    block.setBoundingBox(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 3: {
                    ctx_.southFaceRotation = 2;
                    ctx_.northFaceRotation = 1;
                    ctx_.topFaceRotation = 3;
                    ctx_.bottomFaceRotation = 3;
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f);
                    break;
                }
                case 4: {
                    ctx_.eastFaceRotation = 1;
                    ctx_.westFaceRotation = 2;
                    ctx_.topFaceRotation = 2;
                    ctx_.bottomFaceRotation = 1;
                    block.setBoundingBox(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 5: {
                    ctx_.eastFaceRotation = 2;
                    ctx_.westFaceRotation = 1;
                    ctx_.topFaceRotation = 1;
                    ctx_.bottomFaceRotation = 2;
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);
                }
            }
            cube_.renderBlock(block, x, y, z);
            ctx_.eastFaceRotation = 0;
            ctx_.westFaceRotation = 0;
            ctx_.southFaceRotation = 0;
            ctx_.northFaceRotation = 0;
            ctx_.topFaceRotation = 0;
            ctx_.bottomFaceRotation = 0;
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        } else {
            switch (n2) {
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

void PistonBlockRenderer::renderPistonHeadYAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU)
{

        int n = 108;
        if (ctx_.textureOverride >= 0) {
            n = ctx_.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator& tessellator = render::INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, y2, z1, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x2, y1, z2, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    
}

void PistonBlockRenderer::renderPistonHeadZAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU)
{

        int n = 108;
        if (ctx_.textureOverride >= 0) {
            n = ctx_.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator& tessellator = render::INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, y1, z2, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x2, y2, z1, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    
}

void PistonBlockRenderer::renderPistonHeadXAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU)
{

        int n = 108;
        if (ctx_.textureOverride >= 0) {
            n = ctx_.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator& tessellator = render::INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x2, y1, z1, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x1, y2, z2, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    
}

void PistonBlockRenderer::renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway)
{

        ctx_.skipFaceCulling = true;
        renderPistonHead(block, x, y, z, extendedHalfway);
        ctx_.skipFaceCulling = false;
    
}

bool PistonBlockRenderer::renderPistonHead(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway)
{

        int n = ctx_.blockView->getBlockMeta(x, y, z);
        int n2 = net::minecraft::block::PistonHeadBlock::getFacing(n);
        float f = block.getLuminance(ctx_.blockView, x, y, z);
        float f2 = extendedHalfway ? 1.0f : 0.5f;
        double d = extendedHalfway ? 16.0 : 8.0;
        switch (n2) {
            case 0: {
                ctx_.eastFaceRotation = 3;
                ctx_.westFaceRotation = 3;
                ctx_.southFaceRotation = 3;
                ctx_.northFaceRotation = 3;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.625f, (float)z + 0.625f, f * 0.8f, d);
                renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.375f, (float)z + 0.375f, f * 0.8f, d);
                renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.375f, (float)z + 0.625f, f * 0.6f, d);
                renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.625f, (float)z + 0.375f, f * 0.6f, d);
                break;
            }
            case 1: {
                block.setBoundingBox(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.625f, f * 0.8f, d);
                renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.375f, f * 0.8f, d);
                renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.625f, f * 0.6f, d);
                renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.375f, f * 0.6f, d);
                break;
            }
            case 2: {
                ctx_.southFaceRotation = 1;
                ctx_.northFaceRotation = 2;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.6f, d);
                renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.6f, d);
                renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.5f, d);
                renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + f2, f, d);
                break;
            }
            case 3: {
                ctx_.southFaceRotation = 2;
                ctx_.northFaceRotation = 1;
                ctx_.topFaceRotation = 3;
                ctx_.bottomFaceRotation = 3;
                block.setBoundingBox(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.6f, d);
                renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.6f, d);
                renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.5f, d);
                renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f, d);
                break;
            }
            case 4: {
                ctx_.eastFaceRotation = 1;
                ctx_.westFaceRotation = 2;
                ctx_.topFaceRotation = 2;
                ctx_.bottomFaceRotation = 1;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, f * 0.5f, d);
                renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, f, d);
                renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, f * 0.6f, d);
                renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, f * 0.6f, d);
                break;
            }
            case 5: {
                ctx_.eastFaceRotation = 2;
                ctx_.westFaceRotation = 1;
                ctx_.topFaceRotation = 1;
                ctx_.bottomFaceRotation = 2;
                block.setBoundingBox(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                cube_.renderBlock(block, x, y, z);
                renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, f * 0.5f, d);
                renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, f, d);
                renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, f * 0.6f, d);
                renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, f * 0.6f, d);
            }
        }
        ctx_.eastFaceRotation = 0;
        ctx_.westFaceRotation = 0;
        ctx_.southFaceRotation = 0;
        ctx_.northFaceRotation = 0;
        ctx_.topFaceRotation = 0;
        ctx_.bottomFaceRotation = 0;
        block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return true;
    
}

} // namespace net::minecraft::client::render::block