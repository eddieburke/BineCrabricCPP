#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/CubeBlockRenderer.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders fence block shapes (render type 11).
class FenceBlockRenderer {
public:
    FenceBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube) : ctx_(ctx), cube_(cube) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
    CubeBlockRenderer& cube_;
};

} // namespace net::minecraft::client::render::block
