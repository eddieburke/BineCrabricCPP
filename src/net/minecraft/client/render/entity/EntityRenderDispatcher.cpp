#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
namespace {
using PendingEntry = std::pair<std::type_index, std::function<std::unique_ptr<EntityRenderer>()>>;
std::vector<PendingEntry>& pendingRenderers() {
 static std::vector<PendingEntry> v;
 return v;
}
std::unordered_map<std::type_index, EntityRenderer*>& rendererAliases() {
 static std::unordered_map<std::type_index, EntityRenderer*> map;
 return map;
}
} // namespace
void EntityRenderDispatcher::addPendingRenderer(std::type_index key,
                                                std::function<std::unique_ptr<EntityRenderer>()> factory) {
 for(const auto& [existingKey, _] : pendingRenderers()) {
  if(existingKey == key) {
   assert(false && "EntityRenderDispatcher: duplicate pending renderer registration");
   return;
  }
 }
 pendingRenderers().emplace_back(key, std::move(factory));
}
EntityRenderDispatcher::EntityRenderDispatcher()
    : heldItemRenderer_(std::make_unique<net::minecraft::client::render::item::HeldItemRenderer>()) {
 // Fallback renderers: catch-all chain when no specific renderer registered.
 registerRenderer<net::minecraft::LivingEntity>(
     std::make_unique<LivingEntityRenderer>(new model::BipedEntityModel(), 0.5f));
 registerRenderer<net::minecraft::Entity>(std::make_unique<BoxEntityRenderer>());
 registerRenderer<net::minecraft::PlayerEntity>(std::make_unique<PlayerEntityRenderer>());
}
EntityRenderDispatcher& EntityRenderDispatcher::instance() {
 static EntityRenderDispatcher dispatcher;
 for(auto& [key, factory] : pendingRenderers()) {
  dispatcher.registerRenderer(key, factory());
 }
 pendingRenderers().clear();
 return dispatcher;
}
EntityRenderDispatcher::~EntityRenderDispatcher() = default;
void EntityRenderDispatcher::setHeldItemRenderer(
    std::unique_ptr<net::minecraft::client::render::item::HeldItemRenderer> renderer) {
 heldItemRenderer_ = std::move(renderer);
}
void EntityRenderDispatcher::registerRenderer(std::type_index key, std::unique_ptr<EntityRenderer> renderer) {
 if(renderers_.find(key) != renderers_.end()) {
  assert(false && "EntityRenderDispatcher: duplicate renderer registration");
  return;
 }
 renderer->setDispatcher(this);
 renderers_[key] = std::move(renderer);
 rendererAliases().clear();
}
EntityRenderer* EntityRenderDispatcher::get(std::type_index key) {
 const auto owned = renderers_.find(key);
 if(owned != renderers_.end()) {
  return owned->second.get();
 }
 const auto alias = rendererAliases().find(key);
 if(alias != rendererAliases().end()) {
  return alias->second;
 }
 if(key == std::type_index(typeid(net::minecraft::entity::player::ClientPlayerEntity))) {
  return get(std::type_index(typeid(net::minecraft::PlayerEntity)));
 }
 if(key == std::type_index(typeid(net::minecraft::Entity))) {
  return nullptr;
 }
 const std::optional<std::type_index> parent = net::minecraft::entitySupertype(key);
 if(!parent.has_value()) {
  rendererAliases()[key] = nullptr;
  return nullptr;
 }
 EntityRenderer* resolved = get(*parent);
 rendererAliases()[key] = resolved;
 return resolved;
}
EntityRenderer* EntityRenderDispatcher::get(const net::minecraft::Entity& entity) {
 return get(entity.runtimeType());
}
void EntityRenderDispatcher::init(net::minecraft::World* world,
                                  net::minecraft::client::texture::TextureManager* textureManager,
                                  net::minecraft::client::font::TextRenderer* textRenderer,
                                  net::minecraft::LivingEntity* cameraEntity,
                                  net::minecraft::client::option::GameOptions* options,
                                  float tickDelta) {
 world_ = world;
 textureManager_ = textureManager;
 textRenderer_ = textRenderer;
    cameraEntity_ = cameraEntity;
    options_ = options;
   if(cameraEntity != nullptr && cameraEntity->isSleeping() && world != nullptr) {
  const int blockId = world->getBlockId(
      MathHelper::floor(cameraEntity->x), MathHelper::floor(cameraEntity->y), MathHelper::floor(cameraEntity->z));
  if(blockId == 26) { // Block.BED
   const int meta = world->getBlockMeta(MathHelper::floor(cameraEntity->x),
                                        MathHelper::floor(cameraEntity->y),
                                        MathHelper::floor(cameraEntity->z));
   const int bedDir = meta & 3;
   yaw_ = static_cast<float>(bedDir * 90 + 180);
   pitch_ = 0.0f;
  }
 } else if(cameraEntity != nullptr) {
  yaw_ = cameraEntity->prevYaw + (cameraEntity->yaw - cameraEntity->prevYaw) * tickDelta;
  pitch_ = cameraEntity->prevPitch + (cameraEntity->pitch - cameraEntity->prevPitch) * tickDelta;
 }
 if(cameraEntity != nullptr) {
  x_ = cameraEntity->lastTickX + (cameraEntity->x - cameraEntity->lastTickX) * static_cast<double>(tickDelta);
  y_ = cameraEntity->lastTickY + (cameraEntity->y - cameraEntity->lastTickY) * static_cast<double>(tickDelta);
  z_ = cameraEntity->lastTickZ + (cameraEntity->z - cameraEntity->lastTickZ) * static_cast<double>(tickDelta);
 }
}
void EntityRenderDispatcher::setWorld(net::minecraft::World* world) {
 world_ = world;
 // The old world's entities are gone (or about to be); drop the camera pointer so
 // nothing renders against a freed entity before the next init().
 cameraEntity_ = nullptr;
}
void EntityRenderDispatcher::render(const net::minecraft::Entity& entity,
                                    float tickDelta,
                                    net::minecraft::util::math::MatrixStack& matrices,
                                    const net::minecraft::util::math::Matrix4f& projection) {
 const double x = entity.lastTickX + (entity.x - entity.lastTickX) * static_cast<double>(tickDelta) - offsetX;
 const double y = entity.lastTickY + (entity.y - entity.lastTickY) * static_cast<double>(tickDelta) - offsetY;
 const double z = entity.lastTickZ + (entity.z - entity.lastTickZ) * static_cast<double>(tickDelta) - offsetZ;
 const float yaw = entity.prevYaw + (entity.yaw - entity.prevYaw) * tickDelta;
 // Entity light rides vaUV2 and the vertex colour stays tint-only, so the light is
 // applied exactly once — the same convention terrain already uses. Folding
 // getBrightnessAtEyes into the const colour while pinning the lightmap to (15, 15)
 // double-darkened every entity under a deferred pack: the albedo arrived
 // pre-multiplied by the world light and the pack then lit it again, which at night
 // (no sun term to swamp it) rendered entities pure black.
 // see src/net/minecraft/client/render/block/CubeBlockRenderer.cpp renderSmooth
 // see src/net/minecraft/client/render/shaders/glsl/vanilla/shaders/gbuffers_entities.fsh
 float blockLight = 15.0f;
 float skyLight = 15.0f;
 if(entity.world != nullptr) {
  const int lightX = MathHelper::floor(entity.x);
  const double eyeSpan = (entity.boundingBox.maxY - entity.boundingBox.minY) * 0.66;
  const int lightY =
      MathHelper::floor(entity.y - static_cast<double>(entity.standingEyeHeight) + eyeSpan);
  const int lightZ = MathHelper::floor(entity.z);
  blockLight =
      static_cast<float>(entity.world->getBrightness(net::minecraft::LightType::Block, lightX, lightY, lightZ));
  skyLight =
      static_cast<float>(entity.world->getBrightness(net::minecraft::LightType::Sky, lightX, lightY, lightZ));
 }
 Tessellator::INSTANCE.light(blockLight, skyLight);
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
    render(entity, x, y, z, yaw, tickDelta, matrices, projection);
 }
void EntityRenderDispatcher::render(const net::minecraft::Entity& entity,
                                    double x,
                                    double y,
                                    double z,
                                    float yaw,
                                    float tickDelta,
                                    net::minecraft::util::math::MatrixStack& matrices,
                                    const net::minecraft::util::math::Matrix4f& projection) {
 const std::type_index entityType = entity.runtimeType();
 const std::uint64_t revision = shaderObjectIdRevision();
 if(shaderIdRevision_ != revision) {
  shaderIds_.clear();
  shaderIdRevision_ = revision;
 }
 const auto [shaderIt, inserted] = shaderIds_.try_emplace(entityType, -1);
 if(inserted) {
  const std::string_view shaderName = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity) != nullptr
                                          ? std::string_view("player")
                                          : std::string_view(net::minecraft::entity::EntityRegistry::getIdRef(entity));
  shaderIt->second = resolveShaderObjectId("entity", std::string(shaderName), -1);
 }
 const int entityShaderId = shaderIt->second;
 const render::core::EntityIdScope entityScope(entityShaderId);
 if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::PreEntityRender)) {
  net::minecraft::mod::PreEntityRenderEvent event;
  event.entity = &entity;
  event.entityId = entity.id;
  event.entityRawId = entity.id;
  event.entityType = net::minecraft::entity::EntityRegistry::getIdRef(entity);
  event.tickDelta = tickDelta;
  net::minecraft::mod::runtime::luaHookPreEntityRender(event);
  if(event.canceled) {
   return;
  }
 }
 if(EntityRenderer* renderer = get(entityType); renderer != nullptr) {
  VT_TRACE_COUNTER("EntityRendererInvocations", 1);
  // The renderer composes entity poses onto `matrices` and publishes the
  // composed matrix to the draw camera state before its draws; restore the
  // frame base afterwards (renderers that draw without composing, e.g. the
  // debug hitbox, see the base published here).
  const render::core::ScopedDrawCameraState drawGuard;
  render::core::setDrawPose(matrices.top());
  renderer->render(entity, x, y, z, yaw, tickDelta, matrices, projection);
  renderer->postRender(entity, x, y, z, yaw, tickDelta, matrices, projection);
 }
}
double EntityRenderDispatcher::squaredDistanceTo(double x, double y, double z) const {
 const double dx = x - x_;
 const double dy = y - y_;
 const double dz = z - z_;
 return dx * dx + dy * dy + dz * dz;
}
} // namespace net::minecraft::client::render::entity
