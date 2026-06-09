#pragma once

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace net::minecraft {
class World;
}

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::render::item {
class HeldItemRenderer;
}

namespace net::minecraft::client::render::entity {

class EntityRenderer;

// Faithful port of net.minecraft.client.render.entity.EntityRenderDispatcher.
class EntityRenderDispatcher {
public:
    static EntityRenderDispatcher& instance();

    inline static double offsetX = 0.0;
    inline static double offsetY = 0.0;
    inline static double offsetZ = 0.0;

    EntityRenderDispatcher();
    ~EntityRenderDispatcher();

    void registerRenderer(std::type_index key, std::unique_ptr<EntityRenderer> renderer);

    template <typename EntityType>
    void registerRenderer(std::unique_ptr<EntityRenderer> renderer)
    {
        registerRenderer(std::type_index(typeid(EntityType)), std::move(renderer));
    }

    [[nodiscard]] EntityRenderer* get(const std::type_info& entityType);
    [[nodiscard]] EntityRenderer* get(const net::minecraft::Entity& entity);

    void init(net::minecraft::World* world, net::minecraft::client::texture::TextureManager* textureManager,
        net::minecraft::client::font::TextRenderer* textRenderer, net::minecraft::LivingEntity* cameraEntity,
        net::minecraft::client::option::GameOptions* options, float tickDelta);

    void setWorld(net::minecraft::World* world);

    void render(const net::minecraft::Entity& entity, float tickDelta);
    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta);

    [[nodiscard]] double squaredDistanceTo(double x, double y, double z) const;

    [[nodiscard]] net::minecraft::client::font::TextRenderer* getTextRenderer() const noexcept { return textRenderer_; }

    [[nodiscard]] net::minecraft::World* world() const noexcept { return world_; }

    [[nodiscard]] net::minecraft::client::texture::TextureManager* textureManager() const noexcept
    {
        return textureManager_;
    }

    [[nodiscard]] net::minecraft::LivingEntity* cameraEntity() const noexcept { return cameraEntity_; }

    [[nodiscard]] net::minecraft::client::option::GameOptions& options() const noexcept { return options_; }

    [[nodiscard]] net::minecraft::client::render::item::HeldItemRenderer* heldItemRenderer() noexcept
    {
        return heldItemRenderer_.get();
    }
    [[nodiscard]] const net::minecraft::client::render::item::HeldItemRenderer* heldItemRenderer() const noexcept
    {
        return heldItemRenderer_.get();
    }

    void setHeldItemRenderer(std::unique_ptr<net::minecraft::client::render::item::HeldItemRenderer> renderer);

    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    double x_ = 0.0;
    double y_ = 0.0;
    double z_ = 0.0;

private:
    std::unordered_map<std::type_index, std::unique_ptr<EntityRenderer>> renderers_;
    net::minecraft::World* world_ = nullptr;
    net::minecraft::client::texture::TextureManager* textureManager_ = nullptr;
    net::minecraft::client::font::TextRenderer* textRenderer_ = nullptr;
    net::minecraft::LivingEntity* cameraEntity_ = nullptr;
    net::minecraft::client::option::GameOptions defaultOptions_;
    net::minecraft::client::option::GameOptions& options_;
    std::unique_ptr<net::minecraft::client::render::item::HeldItemRenderer> heldItemRenderer_;
};

} // namespace net::minecraft::client::render::entity
