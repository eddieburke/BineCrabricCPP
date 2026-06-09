#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/CropBlockRenderer.hpp"
#include "net/minecraft/client/render/block/CrossBlockRenderer.hpp"
#include "net/minecraft/client/render/block/TorchBlockRenderer.hpp"

namespace net::minecraft::block {
class Block;
}

namespace net::minecraft::client::render::block {

// Renders block inventory icons (item GUI, held items, entity drops).
class InventoryBlockRenderer {
public:
    InventoryBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces, CrossBlockRenderer& cross,
        CropBlockRenderer& crop, TorchBlockRenderer& torch)
        : ctx_(ctx), faces_(faces), cross_(cross), crop_(crop), torch_(torch)
    {
    }

    void render(net::minecraft::block::Block& block, int metadata, float brightness);

private:
    BlockRenderContext& ctx_;
    BlockFaceRenderer& faces_;
    CrossBlockRenderer& cross_;
    CropBlockRenderer& crop_;
    TorchBlockRenderer& torch_;
};

} // namespace net::minecraft::client::render::block
