#pragma once

#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/CubeBlockRenderer.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders piston body and head block shapes (render types 16, 17).
class PistonBlockRenderer {
public:
    PistonBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube)
        : ctx_(ctx), cube_(cube)
    {
    }

    void renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z);
    bool renderPiston(net::minecraft::block::Block& block, int x, int y, int z, bool extended);
    void renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway);
    bool renderPistonHead(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway);

private:
    void renderPistonHeadYAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU);
    void renderPistonHeadZAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU);
    void renderPistonHeadXAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU);

    BlockRenderContext& ctx_;
    CubeBlockRenderer& cube_;
};

} // namespace net::minecraft::client::render::block
