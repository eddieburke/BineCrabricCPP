#include "net/minecraft/client/render/block/entity/MobSpawnerBlockEntityRenderer.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
namespace net::minecraft::client::render::block::entity {
void MobSpawnerBlockEntityRenderer::render(
    const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z, float tickDelta) {
 const auto* mobSpawner = dynamic_cast<const net::minecraft::block::entity::MobSpawnerBlockEntity*>(&blockEntity);
 if(mobSpawner == nullptr) {
  return;
 }
 render::RenderSystem::pushMatrix();
 render::RenderSystem::translate(static_cast<float>(x) + 0.5f, static_cast<float>(y), static_cast<float>(z) + 0.5f);
 const std::string& entityId = mobSpawner->getSpawnedEntityId();
 auto& cachedEntity = models[entityId];
 if(!cachedEntity) {
  cachedEntity = ::net::minecraft::entity::EntityRegistry::create(entityId, mobSpawner->world);
  if(!cachedEntity) {
   cachedEntity = std::make_unique<net::minecraft::Entity>(mobSpawner->world);
  }
 }
 net::minecraft::Entity* entity = cachedEntity.get();
 if(entity != nullptr) {
  entity->setWorld(mobSpawner->world);
  constexpr float entityScale = 0.4375f;
  render::RenderSystem::translate(0.0f, 0.4f, 0.0f);
  const float spin =
      static_cast<float>((mobSpawner->lastRotation +
                          (mobSpawner->rotation - mobSpawner->lastRotation) * static_cast<double>(tickDelta)) *
                         10.0);
  render::RenderSystem::rotate(spin, 0.0f, 1.0f, 0.0f);
  render::RenderSystem::rotate(-30.0f, 1.0f, 0.0f, 0.0f);
  render::RenderSystem::translate(0.0f, -0.4f, 0.0f);
  render::RenderSystem::scale(entityScale, entityScale, entityScale);
  entity->setPositionAndAnglesKeepPrevAngles(x, y, z, 0.0f, 0.0f);
  render::entity::EntityRenderDispatcher::instance().render(*entity, tickDelta);
 }
 render::RenderSystem::popMatrix();
}
} // namespace net::minecraft::client::render::block::entity
