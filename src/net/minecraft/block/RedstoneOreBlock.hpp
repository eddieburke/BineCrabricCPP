#pragma once
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft {
class World;
}

namespace net::minecraft::block {
class RedstoneOreBlock : public Block {
   public:
    static constexpr int kBlockId = 74;
    static void registerClass();
    bool lit = false;
    RedstoneOreBlock(int id, int textureId, bool litIn);

    [[nodiscard]] int getTickRate() const override {
        return 30;
    }

    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override;
    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onSteppedOn(World* world, int x, int y, int z, net::minecraft::Entity* entity) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

   private:
    void light(World* world, int x, int y, int z);
    static void spawnParticles(World* world, int x, int y, int z);
};
}  // namespace net::minecraft::block
