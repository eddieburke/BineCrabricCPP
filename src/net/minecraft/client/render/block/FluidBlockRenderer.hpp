#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}
namespace net::minecraft::block::material {
class Material;
}

namespace net::minecraft::client::render::block {

// Renders fluid (water/lava) block faces (render type 4).
class FluidBlockRenderer {
public:
    FluidBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces)
        : ctx_(ctx), faces_(faces)
    {
    }

    bool renderFluid(net::minecraft::block::Block& block, int x, int y, int z);

private:
    float getFluidHeight(int x, int y, int z, net::minecraft::block::material::Material& material);

    BlockRenderContext& ctx_;
    BlockFaceRenderer& faces_;
};

} // namespace net::minecraft::client::render::block
