#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders bed block shapes (render type 14).
class BedBlockRenderer {
public:
    BedBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
    BlockFaceRenderer& faces_;
};

} // namespace net::minecraft::client::render::block
