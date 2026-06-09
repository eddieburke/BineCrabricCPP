#pragma once

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::client::render::pipeline {

[[nodiscard]] ItemStack selectedItemOrEmpty(PlayerEntity* player);

} // namespace net::minecraft::client::render::pipeline
