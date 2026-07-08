#pragma once
#include <optional>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::block {
class BedBlock : public Block {
   public:
    static constexpr int kBlockId = 26;
    static void registerClass();
    static constexpr int BED_OFFSETS[4][2] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};
    explicit BedBlock(int id);

    [[nodiscard]] bool isFullCube() const override {
        return false;
    }

    [[nodiscard]] bool isOpaque() const override {
        return false;
    }

    [[nodiscard]] int getRenderType() const override {
        return 14;
    }

    [[nodiscard]] int getTexture(int side, int meta) const override;
    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& /*random*/) const override;

    [[nodiscard]] int getPistonBehavior() const {
        return 1;
    }

    static int getDirection(int meta) {
        return meta & 3;
    }

    static bool isHeadOfBed(int meta) {
        return (meta & 8) != 0;
    }

    static bool isOccupied(int meta) {
        return (meta & 4) != 0;
    }

    static void updateState(World* world, int x, int y, int z, bool occupied);
    static std::optional<Vec3i> findWakeUpPosition(World* world, int x, int y, int z, int skip);
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void dropStacks(World* world, int x, int y, int z, int meta, float luck) override;

   private:
    void setDefaultShape();
};
}  // namespace net::minecraft::block
