#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool FenceBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
 bool rendered = false;
 float barMin = 0.375f;
 float barMax = 0.625f;
 ctx_.setRenderBounds(barMin, 0.0f, barMin, barMax, 1.0f, barMax);
 cube_.renderBlock(block, x, y, z);
 rendered = true;
 bool connectX = false;
 bool connectZ = false;
 if(ctx_.blockView->getBlockId(x - 1, y, z) == block.id || ctx_.blockView->getBlockId(x + 1, y, z) == block.id) {
  connectX = true;
 }
 if(ctx_.blockView->getBlockId(x, y, z - 1) == block.id || ctx_.blockView->getBlockId(x, y, z + 1) == block.id) {
  connectZ = true;
 }
 const bool neighborWest = ctx_.blockView->getBlockId(x - 1, y, z) == block.id;
 const bool neighborEast = ctx_.blockView->getBlockId(x + 1, y, z) == block.id;
 const bool neighborNorth = ctx_.blockView->getBlockId(x, y, z - 1) == block.id;
 const bool neighborSouth = ctx_.blockView->getBlockId(x, y, z + 1) == block.id;
 if(!connectX && !connectZ) {
  connectX = true;
 }
 barMin = 0.4375f;
 barMax = 0.5625f;
 float barMinY = 0.75f;
 float barMaxY = 0.9375f;
 const float xMin = neighborWest ? 0.0f : barMin;
 const float xMax = neighborEast ? 1.0f : barMax;
 const float zMin = neighborNorth ? 0.0f : barMin;
 const float zMax = neighborSouth ? 1.0f : barMax;
 if(connectX) {
  ctx_.setRenderBounds(xMin, barMinY, barMin, xMax, barMaxY, barMax);
  cube_.renderBlock(block, x, y, z);
  rendered = true;
 }
 if(connectZ) {
  ctx_.setRenderBounds(barMin, barMinY, zMin, barMax, barMaxY, zMax);
  cube_.renderBlock(block, x, y, z);
  rendered = true;
 }
 barMinY = 0.375f;
 barMaxY = 0.5625f;
 if(connectX) {
  ctx_.setRenderBounds(xMin, barMinY, barMin, xMax, barMaxY, barMax);
  cube_.renderBlock(block, x, y, z);
  rendered = true;
 }
 if(connectZ) {
  ctx_.setRenderBounds(barMin, barMinY, zMin, barMax, barMaxY, zMax);
  cube_.renderBlock(block, x, y, z);
  rendered = true;
 }
 ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
 return rendered;
}
} // namespace net::minecraft::client::render::block
