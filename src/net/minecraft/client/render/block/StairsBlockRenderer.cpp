#include "net/minecraft/client/render/block/StairsBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool StairsBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    int n = ctx_.blockView->getBlockMeta(x, y, z);
    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    cube_.renderBlock(block, x, y, z);
    if ((n & 3) == 0) {
        block.setBoundingBox(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else if ((n & 3) == 1) {
        block.setBoundingBox(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f);
    } else if ((n & 3) == 2) {
        block.setBoundingBox(0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f);
    } else {
        block.setBoundingBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);
    }
    cube_.renderBlock(block, x, y, z);
    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    return true;
}

} // namespace net::minecraft::client::render::block
