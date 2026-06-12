#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::block {

class SignBlock : public BlockWithEntity {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 63;

static void registerClass();
    SignBlock(int id, bool standing);

    bool standing = false;
    [[nodiscard]] int getRenderType() const override { return -1; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const override
    {
        return std::nullopt;
    }
    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;
};

} // namespace net::minecraft::block
