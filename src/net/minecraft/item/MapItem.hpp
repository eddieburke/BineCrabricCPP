#pragma once

#include "net/minecraft/item/NetworkSyncedItem.hpp"

namespace net::minecraft {
class Packet;
}

namespace net::minecraft::map {
class MapState;
}

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class MapItem : public NetworkSyncedItem {
public:
    static constexpr int kRawId = 102;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    explicit MapItem(int rawId) : NetworkSyncedItem(rawId)
    {
        setMaxCount(1);
    }

    void inventoryTick(ItemStack* stack, World* world, Entity* entity, int slot, bool selected) override;
    void onCraft(ItemStack* stack, World* world, PlayerEntity* player) override;
    [[nodiscard]] Packet* getUpdatePacket(ItemStack* stack, World* world, PlayerEntity* player) override;

    [[nodiscard]] map::MapState* getSavedMapState(ItemStack& stack, World* world);

    [[nodiscard]] static map::MapState* getMapState(std::int16_t mapId, World* world);

private:
    void update(World* world, Entity* entity, map::MapState* mapState);
};

} // namespace net::minecraft::item
