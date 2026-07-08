#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/string.hpp"

namespace net::minecraft::block {
// Registered in Block.cpp.
class CobwebBlock : public Block {
   public:
    CobwebBlock(int id, int textureId) : Block(id, textureId, material::Material::COBWEB) {
    }

    [[nodiscard]] bool isOpaque() const override {
        return false;
    }

    [[nodiscard]] bool isFullCube() const override {
        return false;
    }

    [[nodiscard]] int getRenderType() const override {
        return 1;
    }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/,
                                                                       int /*x*/,
                                                                       int /*y*/,
                                                                       int /*z*/) const override {
        return std::nullopt;
    }

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override {
        return Item::byRawId(31) != nullptr ? Item::byRawId(31)->id : 287;
    }

    void onEntityCollision(World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::Entity* entity) override {
        if (entity != nullptr) {
            entity->slowed = true;
        }
    }
};
}  // namespace net::minecraft::block
