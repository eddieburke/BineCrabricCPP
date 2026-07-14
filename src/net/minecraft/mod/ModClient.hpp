#pragma once
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::client::render::block {
class BlockRenderManager;
}
namespace net::minecraft::mod {
using BlockWorldDraw = bool (*)(client::render::block::BlockRenderManager&, block::Block&, int x, int y, int z);
using BlockInvDraw = void (*)(client::render::block::BlockRenderManager&,
                              block::Block&,
                              int metadata,
                              float brightness);
void registerDraw(int blockId, BlockWorldDraw world, BlockInvDraw inventory = nullptr);
[[nodiscard]] bool drawBlockWorld(
    client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z);
[[nodiscard]] bool drawBlockInventory(client::render::block::BlockRenderManager& manager,
                                      block::Block& block,
                                      int metadata,
                                      float brightness);
} // namespace net::minecraft::mod
