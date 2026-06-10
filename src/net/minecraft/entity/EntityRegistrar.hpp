#pragma once

#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>
#include <typeindex>

namespace net::minecraft::entity::detail {

template<typename EntityType>
void registerVanillaEntity(const char* id, int rawId)
{
    EntityRegistry::registerType(
        std::type_index(typeid(EntityType)),
        id,
        rawId,
        [](World* world) -> std::unique_ptr<Entity> {
            return std::make_unique<EntityType>(world);
        });
}

} // namespace net::minecraft::entity::detail
