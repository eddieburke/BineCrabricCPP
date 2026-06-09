#include "net/minecraft/client/TestInteractionManager.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

#include <array>

namespace net::minecraft::client {
namespace {

std::array<int, 28> creativeInventory()
{
    return {
        1,
        4,
        45,
        3,
        5,
        17,
        18,
        50,
        44,
        20,
        48,
        6,
        37,
        38,
        39,
        40,
        12,
        13,
        19,
        35,
        16,
        15,
        14,
        42,
        41,
        47,
        46,
        49,
    };
}

} // namespace

TestInteractionManager::TestInteractionManager(Minecraft* minecraft)
    : InteractionManager(minecraft)
{
    noTick = true;
}

void TestInteractionManager::preparePlayerRespawn(PlayerEntity* player)
{
    if (player == nullptr || minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    const std::array<int, 28> inventory = creativeInventory();
    for (int i = 0; i < 9; ++i) {
        if (player->inventory.main[i].count <= 0) {
            minecraft->player->inventory.main[i] = ItemStack(inventory[static_cast<std::size_t>(i)]);
        } else {
            minecraft->player->inventory.main[i].count = 1;
        }
    }
}

bool TestInteractionManager::canBeRendered()
{
    return false;
}

void TestInteractionManager::setWorld(World* world)
{
    InteractionManager::setWorld(world);
}

void TestInteractionManager::tick()
{
}

} // namespace net::minecraft::client
