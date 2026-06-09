#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders redstone dust block shapes (render type 5).
class RedstoneDustBlockRenderer {
public:
    explicit RedstoneDustBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
