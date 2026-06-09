#include "net/minecraft/client/render/pipeline/WorldRenderHelpers.hpp"

namespace net::minecraft::client::render::pipeline {

ItemStack selectedItemOrEmpty(PlayerEntity* player)
{
    if (player == nullptr) {
        return {};
    }
    const ItemStack* stack = player->inventory.getSelectedItem();
    return stack != nullptr ? *stack : ItemStack {};
}

} // namespace net::minecraft::client::render::pipeline
