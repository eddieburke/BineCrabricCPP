#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders fire block shapes (render type 3).
class FireBlockRenderer {
public:
    explicit FireBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
