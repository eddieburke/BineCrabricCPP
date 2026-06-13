#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

// Registered in Block.cpp.
class SpawnerBlock : public BlockWithEntity {
public:
    SpawnerBlock(int id, int textureId) : BlockWithEntity(id, textureId, material::Material::STONE) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] bool isOpaque() const override { return false; }

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;
};

} // namespace net::minecraft::block