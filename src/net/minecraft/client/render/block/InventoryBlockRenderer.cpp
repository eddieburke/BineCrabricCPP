#include "net/minecraft/client/render/block/InventoryBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"

namespace net::minecraft::client::render::block {

void InventoryBlockRenderer::render(net::minecraft::block::Block& block, int metadata, float brightness)
{
        float f;
        float f2;
        int n;
        Tessellator& tessellator = render::INSTANCE;
        if (ctx_.inventoryColorEnabled) {
            n = block.getColor(metadata);
            f2 = (float)(n >> 16 & 0xFF) / 255.0f;
            f = (float)(n >> 8 & 0xFF) / 255.0f;
            float f3 = (float)(n & 0xFF) / 255.0f;
            net::minecraft::client::gl::GL11::glColor4f(f2 * brightness, f * brightness, f3 * brightness, 1.0f);
        }
        if ((n = block.getRenderType()) == BlockRenderType::FULL_CUBE || n == BlockRenderType::PISTON) {
            if (n == BlockRenderType::PISTON) {
                metadata = 1;
            }
            block.setupRenderBoundingBox();
            net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 1.0f, 0.0f);
            faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, -1.0f);
            faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, 1.0f);
            faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(-1.0f, 0.0f, 0.0f);
            faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(1.0f, 0.0f, 0.0f);
            faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5, metadata));
            tessellator.draw();
            net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
        } else if (n == BlockRenderType::CROSS) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            cross_.render(block, metadata, -0.5, -0.5, -0.5);
            tessellator.draw();
        } else if (n == BlockRenderType::CACTUS) {
            block.setupRenderBoundingBox();
            net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
            f2 = 0.0625f;
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 1.0f, 0.0f);
            faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, -1.0f);
            tessellator.translate(0.0f, 0.0f, f2);
            faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
            tessellator.translate(0.0f, 0.0f, -f2);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, 1.0f);
            tessellator.translate(0.0f, 0.0f, -f2);
            faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
            tessellator.translate(0.0f, 0.0f, f2);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(-1.0f, 0.0f, 0.0f);
            tessellator.translate(f2, 0.0f, 0.0f);
            faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
            tessellator.translate(-f2, 0.0f, 0.0f);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(1.0f, 0.0f, 0.0f);
            tessellator.translate(-f2, 0.0f, 0.0f);
            faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
            tessellator.translate(f2, 0.0f, 0.0f);
            tessellator.draw();
            net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
        } else if (n == BlockRenderType::CROP) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            crop_.render(block, metadata, -0.5, -0.5, -0.5);
            tessellator.draw();
        } else if (n == BlockRenderType::TORCH) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            torch_.renderTiltedTorch(block, -0.5, -0.5, -0.5, 0.0, 0.0);
            tessellator.draw();
        } else if (n == BlockRenderType::STAIRS) {
            for (int i = 0; i < 2; ++i) {
                if (i == 0) {
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
                }
                if (i == 1) {
                    block.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
                }
                net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
                tessellator.startQuads();
                tessellator.normal(0.0f, -1.0f, 0.0f);
                faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 1.0f, 0.0f);
                faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, -1.0f);
                faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, 1.0f);
                faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(-1.0f, 0.0f, 0.0f);
                faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(1.0f, 0.0f, 0.0f);
                faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
                tessellator.draw();
                net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
            }
        } else if (n == BlockRenderType::FENCE) {
            for (int i = 0; i < 4; ++i) {
                f = 0.125f;
                if (i == 0) {
                    block.setBoundingBox(0.5f - f, 0.0f, 0.0f, 0.5f + f, 1.0f, f * 2.0f);
                }
                if (i == 1) {
                    block.setBoundingBox(0.5f - f, 0.0f, 1.0f - f * 2.0f, 0.5f + f, 1.0f, 1.0f);
                }
                f = 0.0625f;
                if (i == 2) {
                    block.setBoundingBox(0.5f - f, 1.0f - f * 3.0f, -f * 2.0f, 0.5f + f, 1.0f - f, 1.0f + f * 2.0f);
                }
                if (i == 3) {
                    block.setBoundingBox(0.5f - f, 0.5f - f * 3.0f, -f * 2.0f, 0.5f + f, 0.5f - f, 1.0f + f * 2.0f);
                }
                net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
                tessellator.startQuads();
                tessellator.normal(0.0f, -1.0f, 0.0f);
                faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 1.0f, 0.0f);
                faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, -1.0f);
                faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, 1.0f);
                faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(-1.0f, 0.0f, 0.0f);
                faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(1.0f, 0.0f, 0.0f);
                faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
                tessellator.draw();
                net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
            }
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
}

} // namespace net::minecraft::client::render::block
