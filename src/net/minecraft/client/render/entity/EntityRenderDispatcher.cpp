#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"

#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/BoxEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render::entity {

namespace {
using PendingEntry = std::pair<std::type_index, std::function<std::unique_ptr<EntityRenderer>()>>;
std::vector<PendingEntry>& pendingRenderers()
{
    static std::vector<PendingEntry> v;
    return v;
}

std::unordered_map<std::type_index, EntityRenderer*>& rendererAliases()
{
    static std::unordered_map<std::type_index, EntityRenderer*> map;
    return map;
}
}

void EntityRenderDispatcher::addPendingRenderer(std::type_index key,
    std::function<std::unique_ptr<EntityRenderer>()> factory)
{
    for (const auto& [existingKey, _] : pendingRenderers()) {
        if (existingKey == key) {
            std::cerr << "EntityRenderDispatcher: duplicate pending renderer for type "
                      << key.name() << '\n';
            assert(false && "EntityRenderDispatcher: duplicate pending renderer registration");
            return;
        }
    }
    pendingRenderers().emplace_back(key, std::move(factory));
}

EntityRenderDispatcher::EntityRenderDispatcher()
    : options_(defaultOptions_)
    , heldItemRenderer_(std::make_unique<net::minecraft::client::render::item::HeldItemRenderer>())
{
    // Fallback renderers: catch-all chain when no specific renderer registered.
    registerRenderer<net::minecraft::LivingEntity>(
        std::make_unique<LivingEntityRenderer>(new model::BipedEntityModel(), 0.5f));
    registerRenderer<net::minecraft::Entity>(std::make_unique<BoxEntityRenderer>());
    registerRenderer<net::minecraft::PlayerEntity>(std::make_unique<PlayerEntityRenderer>());
}

EntityRenderDispatcher& EntityRenderDispatcher::instance()
{
    static EntityRenderDispatcher dispatcher;
    for (auto& [key, factory] : pendingRenderers()) {
        dispatcher.registerRenderer(key, factory());
    }
    pendingRenderers().clear();
    return dispatcher;
}

EntityRenderDispatcher::~EntityRenderDispatcher() = default;

void EntityRenderDispatcher::setHeldItemRenderer(
    std::unique_ptr<net::minecraft::client::render::item::HeldItemRenderer> renderer)
{
    heldItemRenderer_ = std::move(renderer);
}

void EntityRenderDispatcher::registerRenderer(std::type_index key, std::unique_ptr<EntityRenderer> renderer)
{
    if (renderers_.find(key) != renderers_.end()) {
        std::cerr << "EntityRenderDispatcher: duplicate renderer registration for type "
                  << key.name() << '\n';
        assert(false && "EntityRenderDispatcher: duplicate renderer registration");
        return;
    }
    renderer->setDispatcher(this);
    renderers_[key] = std::move(renderer);
}

EntityRenderer* EntityRenderDispatcher::get(std::type_index key)
{
    const auto owned = renderers_.find(key);
    if (owned != renderers_.end()) {
        return owned->second.get();
    }
    const auto alias = rendererAliases().find(key);
    if (alias != rendererAliases().end()) {
        return alias->second;
    }
    if (key == std::type_index(typeid(net::minecraft::entity::player::ClientPlayerEntity))) {
        return get(std::type_index(typeid(net::minecraft::PlayerEntity)));
    }
    if (key == std::type_index(typeid(net::minecraft::Entity))) {
        return nullptr;
    }
    const std::optional<std::type_index> parent = net::minecraft::entitySupertype(key);
    if (!parent.has_value()) {
        return nullptr;
    }
    EntityRenderer* resolved = get(*parent);
    if (resolved != nullptr) {
        rendererAliases()[key] = resolved;
    }
    return resolved;
}

EntityRenderer* EntityRenderDispatcher::get(const net::minecraft::Entity& entity)
{
    return get(entity.runtimeType());
}

void EntityRenderDispatcher::init(net::minecraft::World* world, net::minecraft::client::texture::TextureManager* textureManager,
    net::minecraft::client::font::TextRenderer* textRenderer, net::minecraft::LivingEntity* cameraEntity,
    net::minecraft::client::option::GameOptions* options, float tickDelta)
{
    world_ = world;
    textureManager_ = textureManager;
    textRenderer_ = textRenderer;
    cameraEntity_ = cameraEntity;
    if (options != nullptr) {
        defaultOptions_ = *options;
    }
    if (cameraEntity != nullptr && cameraEntity->isSleeping() && world != nullptr) {
        const int blockId = world->getBlockId(MathHelper::floor(cameraEntity->x), MathHelper::floor(cameraEntity->y),
            MathHelper::floor(cameraEntity->z));
        if (blockId == 26) { // Block.BED
            const int meta = world->getBlockMeta(MathHelper::floor(cameraEntity->x), MathHelper::floor(cameraEntity->y),
                MathHelper::floor(cameraEntity->z));
            const int bedDir = meta & 3;
            yaw_ = static_cast<float>(bedDir * 90 + 180);
            pitch_ = 0.0f;
        }
    } else if (cameraEntity != nullptr) {
        yaw_ = cameraEntity->prevYaw + (cameraEntity->yaw - cameraEntity->prevYaw) * tickDelta;
        pitch_ = cameraEntity->prevPitch + (cameraEntity->pitch - cameraEntity->prevPitch) * tickDelta;
    }
    if (cameraEntity != nullptr) {
        x_ = cameraEntity->lastTickX + (cameraEntity->x - cameraEntity->lastTickX) * static_cast<double>(tickDelta);
        y_ = cameraEntity->lastTickY + (cameraEntity->y - cameraEntity->lastTickY) * static_cast<double>(tickDelta);
        z_ = cameraEntity->lastTickZ + (cameraEntity->z - cameraEntity->lastTickZ) * static_cast<double>(tickDelta);
    }
}

void EntityRenderDispatcher::setWorld(net::minecraft::World* world)
{
    world_ = world;
}

void EntityRenderDispatcher::render(const net::minecraft::Entity& entity, float tickDelta)
{
    const double x = entity.lastTickX + (entity.x - entity.lastTickX) * static_cast<double>(tickDelta) - offsetX;
    const double y = entity.lastTickY + (entity.y - entity.lastTickY) * static_cast<double>(tickDelta) - offsetY;
    const double z = entity.lastTickZ + (entity.z - entity.lastTickZ) * static_cast<double>(tickDelta) - offsetZ;
    const float yaw = entity.prevYaw + (entity.yaw - entity.prevYaw) * tickDelta;
    const float brightness = entity.getBrightnessAtEyes(tickDelta);
    gl::GL11::glColor3f(brightness, brightness, brightness);
    render(entity, x, y, z, yaw, tickDelta);
}

void EntityRenderDispatcher::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
    float tickDelta)
{
    if (EntityRenderer* renderer = get(entity); renderer != nullptr) {
        renderer->render(entity, x, y, z, yaw, tickDelta);
        renderer->postRender(entity, x, y, z, yaw, tickDelta);
    }
}

double EntityRenderDispatcher::squaredDistanceTo(double x, double y, double z) const
{
    const double dx = x - x_;
    const double dy = y - y_;
    const double dz = z - z_;
    return dx * dx + dy * dy + dz * dz;
}

} // namespace net::minecraft::client::render::entity
