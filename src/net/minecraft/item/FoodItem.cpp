#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/FoodItem.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

FoodItem::FoodItem(int rawId, int healAmount, bool wolfFood)
    : Item(rawId),
      healAmount_(healAmount),
      wolfFood_(wolfFood)
{
    setMaxCount(1);
}

ItemStack* FoodItem::use(ItemStack* stack, World* /*world*/, PlayerEntity* user)
{
    if (stack != nullptr) {
        --stack->count;
    }
    if (user != nullptr) {
        user->heal(healAmount_);
    }
    return stack;
}

namespace {

void FoodItem::registerClass()
{
    static FoodItem APPLE(4, 4, false);
    APPLE.setTexturePosition(10, 0)->setTranslationKey("apple");
    Item::APPLE = &APPLE;

    static FoodItem BREAD(41, 5, false);
    BREAD.setTexturePosition(9, 2)->setTranslationKey("bread");
    Item::BREAD = &BREAD;

    static FoodItem RAW_PORKCHOP(63, 3, true);
    RAW_PORKCHOP.setTexturePosition(7, 5)->setTranslationKey("porkchopRaw");
    Item::RAW_PORKCHOP = &RAW_PORKCHOP;

    static FoodItem COOKED_PORKCHOP(64, 8, true);
    COOKED_PORKCHOP.setTexturePosition(8, 5)->setTranslationKey("porkchopCooked");
    Item::COOKED_PORKCHOP = &COOKED_PORKCHOP;

    static FoodItem GOLDEN_APPLE(66, 42, false);
    GOLDEN_APPLE.setTexturePosition(11, 0)->setTranslationKey("appleGold");
    Item::GOLDEN_APPLE = &GOLDEN_APPLE;

    static FoodItem RAW_FISH(93, 2, false);
    RAW_FISH.setTexturePosition(9, 5)->setTranslationKey("fishRaw");
    Item::RAW_FISH = &RAW_FISH;

    static FoodItem COOKED_FISH(94, 5, false);
    COOKED_FISH.setTexturePosition(10, 5)->setTranslationKey("fishCooked");
    Item::COOKED_FISH = &COOKED_FISH;
}




static ::net::minecraft::registry::RegisterItem<FoodItem> autoReg(4);
} // namespace

} // namespace net::minecraft::item
