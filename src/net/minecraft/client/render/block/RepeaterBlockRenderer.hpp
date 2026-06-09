#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/CubeBlockRenderer.hpp"
#include "net/minecraft/client/render/block/TorchBlockRenderer.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders repeater block shapes (render type 15).
class RepeaterBlockRenderer {
public:
    RepeaterBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube, TorchBlockRenderer& torch)
        : ctx_(ctx), cube_(cube), torch_(torch)
    {
    }

    bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
    CubeBlockRenderer& cube_;
    TorchBlockRenderer& torch_;
};

} // namespace net::minecraft::client::render::block
