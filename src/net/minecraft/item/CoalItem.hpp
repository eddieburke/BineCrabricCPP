#pragma once

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

class CoalItem : public Item {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 7;

static void registerClass();
    static void registerSmeltingRecipes();
    explicit CoalItem(int rawId) : Item(rawId)
    {
        setHasSubtypes(true);
        setMaxDamage(0);
    }

    [[nodiscard]] std::string getTranslationKey(const ItemStack* stack) const override
    {
        return stack != nullptr && stack->getDamage() == 1 ? "item.charcoal" : "item.coal";
    }
};

} // namespace net::minecraft::item
