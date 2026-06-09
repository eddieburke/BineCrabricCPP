#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders cross-shaped plant block shapes (render type 1).
class CrossBlockRenderer {
public:
    explicit CrossBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}

    bool render(net::minecraft::block::Block& block, int x, int y, int z);
    void render(net::minecraft::block::Block& block, int metadata, double x, double y, double z);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
