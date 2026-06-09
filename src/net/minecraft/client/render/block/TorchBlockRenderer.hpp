#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders torch block shapes (render type 2).
class TorchBlockRenderer {
public:
    explicit TorchBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);
    void renderTiltedTorch(net::minecraft::block::Block& block, double x, double y, double z, double xTilt, double zTilt);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
