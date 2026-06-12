#pragma once

#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include <memory>
#include <typeindex>

namespace net::minecraft::registry {

namespace detail {

template<typename EntityType>
concept HasEntityClientRenderer = requires {
    typename EntityType::ClientRenderer;
    {
        EntityType::ClientRenderer::create()
    } -> std::same_as<std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer>>;
};

template<typename EntityType>
    requires HasEntityClientRenderer<EntityType>
int entityClientRendererPriority()
{
    if constexpr (requires { EntityType::kEntityId; }) {
        return EntityType::kEntityId;
    }
    static int nextPriority = 1000;
    return nextPriority++;
}

template<typename EntityType>
    requires HasEntityClientRenderer<EntityType>
void registerEntityClientRenderer()
{
    ::net::minecraft::client::render::entity::EntityRenderDispatcher::addPendingRenderer(
        std::type_index(typeid(EntityType)),
        []() -> std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> {
            return EntityType::ClientRenderer::create();
        });
}

} // namespace detail

template<typename EntityType>
    requires detail::HasEntityClientRenderer<EntityType>
struct RegisterEntityRenderer {
    RegisterEntityRenderer()
    {
        Registry::addCustom(
            kClientRendererRegistrarBase + detail::entityClientRendererPriority<EntityType>(),
            &detail::registerEntityClientRenderer<EntityType>);
    }
};

} // namespace net::minecraft::registry
