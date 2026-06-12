#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class LockedChestBlock : public Block {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 95;

static void registerClass();
    using Block::canPlaceAt;
    explicit LockedChestBlock(int id);

    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] int getTexture(int side) const override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
};

} // namespace net::minecraft::block
