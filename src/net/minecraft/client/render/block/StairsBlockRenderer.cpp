#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool StairsBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
  const int n = ctx_.blockView->getBlockMeta(x, y, z);
  bool drewAny = false;
  if(n == 0) {
    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
    ctx_.setRenderBounds(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
  } else if(n == 1) {
    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
    ctx_.setRenderBounds(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
  } else if(n == 2) {
    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
    ctx_.setRenderBounds(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
  } else if(n == 3) {
    ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
    ctx_.setRenderBounds(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
    drewAny = cube_.renderBlock(block, x, y, z) || drewAny;
  }
  ctx_.setRenderBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
  return drewAny;
}
} // namespace net::minecraft::client::render::block
