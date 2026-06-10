#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

class JukeboxBlock : public BlockWithEntity {
public:
    static void registerClass();
    JukeboxBlock(int id, int textureId);

    [[nodiscard]] int getTexture(int side) const override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void insertRecord(World* world, int x, int y, int z, int recordId);
    void tryEjectRecord(World* world, int x, int y, int z);
    void onBreak(World* world, int x, int y, int z) override;
    void dropStacks(World* world, int x, int y, int z, int meta, float luck) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;
};

} // namespace net::minecraft::block
