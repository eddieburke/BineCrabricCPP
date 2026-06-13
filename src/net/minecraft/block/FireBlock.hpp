#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft {
class BlockView;
class World;
}

namespace net::minecraft::block {

class FireBlock : public Block {
public:
    static constexpr int kBlockId = 51;

static void registerClass();
    using Block::canPlaceAt;
    FireBlock(int id, int textureId);

    void init() override;
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 3; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getTickRate() const override { return 40; }
    [[nodiscard]] bool hasCollision() const override { return false; }
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const override
    {
        return std::nullopt;
    }

    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void onPlaced(World* world, int x, int y, int z) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

    static bool isFlammable(const BlockView* blockView, int x, int y, int z);
    [[nodiscard]] int getBurnChance(World* world, int x, int y, int z, int currentChance) const;

private:
    static bool isFlammableBlock(const BlockView* blockView, int x, int y, int z);
    void registerFlammableBlock(int block, int burnChance, int spreadChance);
    void trySpreadingFire(World* world, int x, int y, int z, int spreadFactor,
        JavaRandom& random, int currentAge) const;
    [[nodiscard]] bool areBlocksAroundFlammable(World* world, int x, int y, int z) const;
    [[nodiscard]] int getBurnChanceAt(World* world, int x, int y, int z) const;

    std::array<int, Block::BLOCK_COUNT> burnChances_{};
    std::array<int, Block::BLOCK_COUNT> spreadChances_{};
    bool flammablesInitialized_ = false;
};

} // namespace net::minecraft::block
