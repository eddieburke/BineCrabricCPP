#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

class OreBlock : public Block {
public:
    static void registerClass();
    OreBlock(int id, int textureId) : Block(id, textureId, material::Material::STONE) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override
    {
        if (Block::COAL_ORE != nullptr && id == Block::COAL_ORE->id) {
            return Item::COAL != nullptr ? Item::COAL->id : 263;
        }
        if (Block::DIAMOND_ORE != nullptr && id == Block::DIAMOND_ORE->id) {
            return Item::DIAMOND != nullptr ? Item::DIAMOND->id : 264;
        }
        if (Block::LAPIS_ORE != nullptr && id == Block::LAPIS_ORE->id) {
            return Item::DYE != nullptr ? Item::DYE->id : 351;
        }
        return id;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override
    {
        if (Block::LAPIS_ORE != nullptr && id == Block::LAPIS_ORE->id) {
            return 4 + random.nextInt(5);
        }
        return 1;
    }

protected:
    [[nodiscard]] int getDroppedItemMeta(int /*blockMeta*/) const override
    {
        if (Block::LAPIS_ORE != nullptr && id == Block::LAPIS_ORE->id) {
            return 4;
        }
        return 0;
    }
};

} // namespace net::minecraft::block
