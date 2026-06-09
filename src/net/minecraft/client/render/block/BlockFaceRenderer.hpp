#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"

namespace net::minecraft::client::render::block {

// Emits the six axis-aligned quads of a block face into the Tessellator.
// Reads texture-override, U-flip, per-face rotation and AO colours from the
// shared BlockRenderContext. Extracted verbatim from the beta 1.7.3 RenderBlocks
// god-class; the UV math is preserved exactly.
class BlockFaceRenderer {
public:
    explicit BlockFaceRenderer(BlockRenderContext& ctx)
        : ctx_(ctx)
    {
    }

    void renderBottomFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
    void renderTopFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
    void renderEastFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
    void renderWestFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
    void renderNorthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
    void renderSouthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);

private:
    BlockRenderContext& ctx_;
};

} // namespace net::minecraft::client::render::block
