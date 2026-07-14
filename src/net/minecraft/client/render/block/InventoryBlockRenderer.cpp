#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
namespace net::minecraft::client::render::block {
namespace {
void applyInventoryColor(float red, float green, float blue, float brightness) {
  net::minecraft::client::gl::color4f(
      red * brightness, green * brightness, blue * brightness, 1.0f);
}
void drawInventoryCubeFaces(Tessellator& tessellator,
                            BlockFaceRenderer& faces,
                            net::minecraft::block::Block& block,
                            int metadata,
                            float red,
                            float green,
                            float blue,
                            float brightness) {
  applyInventoryColor(red, green, blue, brightness);
  tessellator.startQuads();
  tessellator.normal(0.0f, -1.0f, 0.0f);
  faces.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0, metadata));
  tessellator.normal(0.0f, 1.0f, 0.0f);
  faces.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1, metadata));
  tessellator.normal(0.0f, 0.0f, -1.0f);
  faces.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2, metadata));
  tessellator.normal(0.0f, 0.0f, 1.0f);
  faces.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3, metadata));
  tessellator.normal(-1.0f, 0.0f, 0.0f);
  faces.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4, metadata));
  tessellator.normal(1.0f, 0.0f, 0.0f);
  faces.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5, metadata));
  tessellator.draw();
}
} // namespace
void InventoryBlockRenderer::render(net::minecraft::block::Block& block, int metadata, float brightness) {
  const int renderType = block.getRenderType();
  Tessellator& tessellator = *ctx_.tess;
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
  if(ctx_.inventoryColorEnabled) {
    const int blockColor = block.getColor(metadata);
    red = static_cast<float>(blockColor >> 16 & 0xFF) / 255.0f;
    green = static_cast<float>(blockColor >> 8 & 0xFF) / 255.0f;
    blue = static_cast<float>(blockColor & 0xFF) / 255.0f;
  }
  ctx_.faceState.useAo = false;
  if(renderType == BlockRenderType::FULL_CUBE || renderType == BlockRenderType::PISTON) {
    if(renderType == BlockRenderType::PISTON) {
      metadata = 1;
    }
    block.setupRenderBoundingBox();
    ctx_.renderBounds = block.getCollisionShapeLocal();
    net::minecraft::client::gl::translatef(-0.5f, -0.5f, -0.5f);
    drawInventoryCubeFaces(tessellator, faces_, block, metadata, red, green, blue, brightness);
    net::minecraft::client::gl::translatef(0.5f, 0.5f, 0.5f);
  } else if(renderType == BlockRenderType::CROSS) {
    applyInventoryColor(red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    cross_.render(block, metadata, -0.5, -0.5, -0.5);
    tessellator.draw();
  } else if(renderType == BlockRenderType::CACTUS) {
    block.setupRenderBoundingBox();
    ctx_.renderBounds = block.getCollisionShapeLocal();
    net::minecraft::client::gl::translatef(-0.5f, -0.5f, -0.5f);
    const float faceInset = 0.0625f;
    applyInventoryColor(red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
    tessellator.normal(0.0f, 1.0f, 0.0f);
    faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
    tessellator.normal(0.0f, 0.0f, -1.0f);
    tessellator.translate(0.0f, 0.0f, faceInset);
    faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
    tessellator.translate(0.0f, 0.0f, -faceInset);
    tessellator.normal(0.0f, 0.0f, 1.0f);
    tessellator.translate(0.0f, 0.0f, -faceInset);
    faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
    tessellator.translate(0.0f, 0.0f, faceInset);
    tessellator.normal(-1.0f, 0.0f, 0.0f);
    tessellator.translate(faceInset, 0.0f, 0.0f);
    faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
    tessellator.translate(-faceInset, 0.0f, 0.0f);
    tessellator.normal(1.0f, 0.0f, 0.0f);
    tessellator.translate(-faceInset, 0.0f, 0.0f);
    faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
    tessellator.translate(faceInset, 0.0f, 0.0f);
    tessellator.draw();
    net::minecraft::client::gl::translatef(0.5f, 0.5f, 0.5f);
  } else if(renderType == BlockRenderType::CROP) {
    applyInventoryColor(red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    crop_.render(block, metadata, -0.5, -0.5, -0.5);
    tessellator.draw();
  } else if(renderType == BlockRenderType::TORCH) {
    applyInventoryColor(red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    torch_.renderTiltedTorch(block, -0.5, -0.5, -0.5, 0.0, 0.0);
    tessellator.draw();
  } else if(renderType == BlockRenderType::STAIRS) {
    for(int i = 0; i < 2; ++i) {
      if(i == 0) {
        ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
      }
      if(i == 1) {
        ctx_.setRenderBounds(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
      }
      net::minecraft::client::gl::translatef(-0.5f, -0.5f, -0.5f);
      drawInventoryCubeFaces(tessellator, faces_, block, metadata, red, green, blue, brightness);
      net::minecraft::client::gl::translatef(0.5f, 0.5f, 0.5f);
    }
  } else if(renderType == BlockRenderType::FENCE) {
    for(int i = 0; i < 4; ++i) {
      float postHalfWidth = 0.125f;
      if(i == 0) {
        ctx_.setRenderBounds(
            0.5f - postHalfWidth, 0.0f, 0.0f, 0.5f + postHalfWidth, 1.0f, postHalfWidth * 2.0f);
      }
      if(i == 1) {
        ctx_.setRenderBounds(
            0.5f - postHalfWidth, 0.0f, 1.0f - postHalfWidth * 2.0f, 0.5f + postHalfWidth, 1.0f, 1.0f);
      }
      postHalfWidth = 0.0625f;
      if(i == 2) {
        ctx_.setRenderBounds(0.5f - postHalfWidth,
                             1.0f - postHalfWidth * 3.0f,
                             -postHalfWidth * 2.0f,
                             0.5f + postHalfWidth,
                             1.0f - postHalfWidth,
                             1.0f + postHalfWidth * 2.0f);
      }
      if(i == 3) {
        ctx_.setRenderBounds(0.5f - postHalfWidth,
                             0.5f - postHalfWidth * 3.0f,
                             -postHalfWidth * 2.0f,
                             0.5f + postHalfWidth,
                             0.5f - postHalfWidth,
                             1.0f + postHalfWidth * 2.0f);
      }
      net::minecraft::client::gl::translatef(-0.5f, -0.5f, -0.5f);
      drawInventoryCubeFaces(tessellator, faces_, block, metadata, red, green, blue, brightness);
      net::minecraft::client::gl::translatef(0.5f, 0.5f, 0.5f);
    }
    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
  }
}
} // namespace net::minecraft::client::render::block
