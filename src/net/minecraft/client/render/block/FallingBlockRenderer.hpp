#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft {
class World;
}

namespace net::minecraft::client::render::block {

// Renders falling block entities (sand, gravel) with per-face world luminance.
class FallingBlockRenderer {
public:
    FallingBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces)
        : ctx_(ctx), faces_(faces)
    {
    }

    void renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
    BlockFaceRenderer& faces_;
};

} // namespace net::minecraft::client::render::block
