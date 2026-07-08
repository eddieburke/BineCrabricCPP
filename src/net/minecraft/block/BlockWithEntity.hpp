#pragma once
#include <cstddef>
#include <memory>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"

namespace net::minecraft::block {
class BlockWithEntity : public Block {
   public:
    BlockWithEntity(int id, Material& material) : Block(id, material) {
        BLOCKS_WITH_ENTITY[static_cast<std::size_t>(id)] = true;
    }

    BlockWithEntity(int id, int textureId, Material& material) : Block(id, textureId, material) {
        BLOCKS_WITH_ENTITY[static_cast<std::size_t>(id)] = true;
    }

    void onPlaced(World* world, int x, int y, int z) override;
    void onBreak(World* world, int x, int y, int z) override;

   protected:
    virtual std::unique_ptr<entity::BlockEntity> createBlockEntity() = 0;
};
}  // namespace net::minecraft::block