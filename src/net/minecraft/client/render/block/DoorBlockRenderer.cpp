#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool DoorBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
  Tessellator& tessellator = *ctx_.tess;
  bool drewAnyFace = false;
  const float downShade = 0.5f;
  const float upShade = 1.0f;
  const float horizShade = 0.8f;
  const float nsShade = 0.6f;
  const float selfBrightness = block.getLuminance(ctx_.blockView, x, y, z);
  float brightness = block.getLuminance(ctx_.blockView, x, y - 1, z);
  if(ctx_.renderBounds.minY > 0.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(downShade * brightness, downShade * brightness, downShade * brightness);
  faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
  drewAnyFace = true;
  brightness = block.getLuminance(ctx_.blockView, x, y + 1, z);
  if(ctx_.renderBounds.maxY < 1.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(upShade * brightness, upShade * brightness, upShade * brightness);
  faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
  drewAnyFace = true;
  brightness = block.getLuminance(ctx_.blockView, x, y, z - 1);
  if(ctx_.renderBounds.minZ > 0.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(horizShade * brightness, horizShade * brightness, horizShade * brightness);
  int texture = block.getTextureId(ctx_.blockView, x, y, z, 2);
  if(texture < 0) {
    ctx_.flipTextureHorizontally = true;
    texture = -texture;
  }
  faces_.renderEastFace(block, x, y, z, texture);
  drewAnyFace = true;
  ctx_.flipTextureHorizontally = false;
  brightness = block.getLuminance(ctx_.blockView, x, y, z + 1);
  if(ctx_.renderBounds.maxZ < 1.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(horizShade * brightness, horizShade * brightness, horizShade * brightness);
  texture = block.getTextureId(ctx_.blockView, x, y, z, 3);
  if(texture < 0) {
    ctx_.flipTextureHorizontally = true;
    texture = -texture;
  }
  faces_.renderWestFace(block, x, y, z, texture);
  drewAnyFace = true;
  ctx_.flipTextureHorizontally = false;
  brightness = block.getLuminance(ctx_.blockView, x - 1, y, z);
  if(ctx_.renderBounds.minX > 0.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(nsShade * brightness, nsShade * brightness, nsShade * brightness);
  texture = block.getTextureId(ctx_.blockView, x, y, z, 4);
  if(texture < 0) {
    ctx_.flipTextureHorizontally = true;
    texture = -texture;
  }
  faces_.renderNorthFace(block, x, y, z, texture);
  drewAnyFace = true;
  ctx_.flipTextureHorizontally = false;
  brightness = block.getLuminance(ctx_.blockView, x + 1, y, z);
  if(ctx_.renderBounds.maxX < 1.0) {
    brightness = selfBrightness;
  }
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block.id)] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(nsShade * brightness, nsShade * brightness, nsShade * brightness);
  texture = block.getTextureId(ctx_.blockView, x, y, z, 5);
  if(texture < 0) {
    ctx_.flipTextureHorizontally = true;
    texture = -texture;
  }
  faces_.renderSouthFace(block, x, y, z, texture);
  drewAnyFace = true;
  ctx_.flipTextureHorizontally = false;
  return drewAnyFace;
}
} // namespace net::minecraft::client::render::block
