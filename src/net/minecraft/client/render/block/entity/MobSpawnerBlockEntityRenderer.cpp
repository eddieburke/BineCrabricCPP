#include "net/minecraft/client/render/block/entity/MobSpawnerBlockEntityRenderer.hpp"

#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"

namespace net::minecraft::client::render::block::entity {

void MobSpawnerBlockEntityRenderer::render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z, float tickDelta)
{
    const auto* mobSpawner = dynamic_cast<const net::minecraft::block::entity::MobSpawnerBlockEntity*>(&blockEntity);
    if (mobSpawner == nullptr) {
        return;
    }

    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(x) + 0.5f, static_cast<float>(y), static_cast<float>(z) + 0.5f);

    const std::string& entityId = mobSpawner->getSpawnedEntityId();
    auto& cachedEntity = models[entityId];
    if (!cachedEntity) {
        cachedEntity = ::net::minecraft::entity::EntityRegistry::create(entityId, mobSpawner->world);
        if (!cachedEntity) {
            cachedEntity = std::make_unique<net::minecraft::Entity>(mobSpawner->world);
        }
    }

    net::minecraft::Entity* entity = cachedEntity.get();
    if (entity != nullptr) {
        entity->setWorld(mobSpawner->world);
        constexpr float entityScale = 0.4375f;
        gl::GL11::glTranslatef(0.0f, 0.4f, 0.0f);
        const float spin = static_cast<float>((mobSpawner->lastRotation + (mobSpawner->rotation - mobSpawner->lastRotation) * static_cast<double>(tickDelta)) * 10.0);
        gl::GL11::glRotatef(spin, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glTranslatef(0.0f, -0.4f, 0.0f);
        gl::GL11::glScalef(entityScale, entityScale, entityScale);
        entity->setPositionAndAnglesKeepPrevAngles(x, y, z, 0.0f, 0.0f);
        render::entity::EntityRenderDispatcher::instance().render(*entity, tickDelta);
    }

    gl::GL11::glPopMatrix();
}

} // namespace net::minecraft::client::render::block::entity
