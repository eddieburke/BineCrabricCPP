#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders full-cube and cactus block shapes (render types 0 and 13).
// Owns the AO smooth-lighting (renderSmooth) and flat-lighting (renderFlat)
// paths extracted from the beta 1.7.3 BlockRenderManager.
class CubeBlockRenderer {
public:
    CubeBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces, bool& fancyGraphics)
        : ctx_(ctx), faces_(faces), fancyGraphics_(fancyGraphics)
    {
    }

    bool renderBlock(net::minecraft::block::Block& block, int x, int y, int z);
    bool renderSmooth(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);
    bool renderFlat(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);
    bool renderCactus(net::minecraft::block::Block& block, int x, int y, int z);
    bool renderCactus(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);

private:
    BlockRenderContext& ctx_;
    BlockFaceRenderer& faces_;
    bool& fancyGraphics_;
};

} // namespace net::minecraft::client::render::block
