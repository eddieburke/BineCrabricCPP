#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/Item.hpp"

#include <array>
#include <string>

namespace net::minecraft::recipe {
class CraftingRecipeManager;
class SmeltingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class DyeItem : public Item {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 95;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    static void registerSmeltingRecipes();
    static constexpr std::array<const char*, 16> names {
        "black", "red", "green", "brown", "blue", "purple", "cyan", "silver",
        "gray", "pink", "lime", "yellow", "lightBlue", "magenta", "orange", "white"};
    static constexpr std::array<int, 16> colors {
        0x1E1B1B, 11743532, 3887386, 5320730, 2437522, 8073150, 2651799, 2651799,
        0x434343, 14188952, 4312372, 14602026, 6719955, 12801229, 15435844, 0xF0F0F0};

    explicit DyeItem(int rawId);
    [[nodiscard]] int getTextureId(int damage) const override;
    [[nodiscard]] std::string getTranslationKey(const ItemStack* stack) const override;
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
    void useOnEntity(ItemStack* stack, LivingEntity* entity) override;
};

} // namespace net::minecraft::item
