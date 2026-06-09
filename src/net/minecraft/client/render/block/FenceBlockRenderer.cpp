#include "net/minecraft/client/render/block/FenceBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool FenceBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    float f2 = 0.375f;
    float f3 = 0.625f;
    block.setBoundingBox(f2, 0.0f, f2, f3, 1.0f, f3);
    cube_.renderBlock(block, x, y, z);
    bool bl3 = false;
    bool bl4 = false;
    if (ctx_.blockView->getBlockId(x - 1, y, z) == block.id || ctx_.blockView->getBlockId(x + 1, y, z) == block.id) {
        bl3 = true;
    }
    if (ctx_.blockView->getBlockId(x, y, z - 1) == block.id || ctx_.blockView->getBlockId(x, y, z + 1) == block.id) {
        bl4 = true;
    }
    float f4 = 0.0625f;
    float f5 = 0.5f;
    if (bl3) {
        block.setBoundingBox(0.0f, f5 + f4 * 2.0f, f3 - f4 * 2.0f, f2, f5 + f4 * 4.0f, f2 + f4 * 2.0f);
        cube_.renderBlock(block, x, y, z);
        block.setBoundingBox(0.0f, f4 * 3.0f, f3 - f4 * 2.0f, f2, f4 * 5.0f, f2 + f4 * 2.0f);
        cube_.renderBlock(block, x, y, z);
    }
    if (bl4) {
        block.setBoundingBox(f3 - f4 * 2.0f, f5 + f4 * 2.0f, 0.0f, f2 + f4 * 2.0f, f5 + f4 * 4.0f, f2);
        cube_.renderBlock(block, x, y, z);
        block.setBoundingBox(f3 - f4 * 2.0f, f4 * 3.0f, 0.0f, f2 + f4 * 2.0f, f4 * 5.0f, f2);
        cube_.renderBlock(block, x, y, z);
    }
    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    return bl3 || bl4;
}

} // namespace net::minecraft::client::render::block
