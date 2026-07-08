#pragma once
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/DyeItem.hpp"

namespace net::minecraft::item {
class WoolBlockItem : public BlockItem {
   public:
    explicit WoolBlockItem(int rawId) : BlockItem(rawId) {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    [[nodiscard]] int getPlacementMetadata(int meta) const override {
        return meta;
    }

    [[nodiscard]] int getTextureId(int damage) const override {
        return Block::WOOL != nullptr ? Block::WOOL->getTexture(2, block::WoolBlock::getBlockMeta(damage))
                                      : BlockItem::getTextureId(damage);
    }

    [[nodiscard]] std::string getTranslationKey(const ItemStack* stack) const override {
        const int damage = stack != nullptr ? stack->getDamage() : 0;
        const auto index = static_cast<std::size_t>(block::WoolBlock::getBlockMeta(damage)) % DyeItem::names.size();
        return BlockItem::getTranslationKey(stack) + "." + std::string(DyeItem::names[index]);
    }
};
}  // namespace net::minecraft::item

namespace net::minecraft {
using WoolBlockItem = item::WoolBlockItem;
}
