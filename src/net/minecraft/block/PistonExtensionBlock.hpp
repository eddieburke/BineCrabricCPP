#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockView.hpp"

#include <memory>

namespace net::minecraft::block {

class PistonExtensionBlock : public BlockWithEntity {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 36;

static void registerClass();
    explicit PistonExtensionBlock(int id) : BlockWithEntity(id, material::Material::PISTON) { setHardness(-1.0f); }

    [[nodiscard]] int getRenderType() const override { return -1; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override;
    void onBreak(World* world, int x, int y, int z) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void dropStacks(World* world, int x, int y, int z, int meta, float luck) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;

    [[nodiscard]] std::optional<net::minecraft::Box> getPushedBlockCollisionShape(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        float sizeMultiplier,
        int facing) const;

    static std::unique_ptr<entity::BlockEntity> createPistonBlockEntity(
        int blockId, int blockMeta, int facing, bool extending, bool source);

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;
};

} // namespace net::minecraft::block
