#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::block {
class RailBlock;
}

namespace net::minecraft::client::render::block {

// Renders rail block shapes (render type 9).
class RailBlockRenderer {
public:
    explicit RailBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}

    bool render(net::minecraft::block::RailBlock& rail, int x, int y, int z);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
