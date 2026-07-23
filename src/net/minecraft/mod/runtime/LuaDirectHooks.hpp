#pragma once
#include <functional>
#include <string>
#include <string_view>
#include "net/minecraft/mod/runtime/LuaEventId.hpp"
#include "net/minecraft/mod/events/ClientEvents.hpp"
#include "net/minecraft/mod/events/EntityEvents.hpp"
#include "net/minecraft/mod/events/TileEntityEvents.hpp"
#include "net/minecraft/mod/events/WorldEvents.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/LuaEventId.hpp"
namespace net::minecraft {
class World;
namespace entity {
class Entity;
class LivingEntity;
namespace player {
class PlayerEntity;
}
} // namespace entity
} // namespace net::minecraft
namespace net::minecraft::client::render {
struct FrameRenderCamera;
}
namespace net::minecraft::world::gen {
struct ChunkGenerationEvent;
enum class ChunkStage;
} // namespace net::minecraft::world::gen
namespace net::minecraft::mod {
struct ScreenRegionEvent;
struct ScreenUiEvent;
} // namespace net::minecraft::mod
namespace net::minecraft::mod::runtime {
struct LuaScreenEvent;
}
namespace net::minecraft::mod {
struct CameraSetupEvent {
 entity::LivingEntity* camera = nullptr;
 float tickDelta = 0.0f;
 client::render::FrameRenderCamera* frame = nullptr;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 float yaw = 0.0f;
 float pitch = 0.0f;
 float roll = 0.0f;
 bool customView = false;
 bool hideFirstPersonHand = false;
};
struct RenderFrameEvent {
 float tickDelta = 0.0f;
};
struct FovEvent {
 entity::LivingEntity* camera = nullptr;
 float tickDelta = 0.0f;
 float fov = 70.0f;
};
enum class WorldRenderStage {
 Sky,
 Stars,
 OpaqueTerrain,
 Entities,
 LitParticles,
 Particles,
 TranslucentTerrain,
 Weather,
 Clouds,
 Hand,
 Framebuffer,
};
enum class RenderHookMoment {
 Before,
 After,
};
struct WorldRenderEvent {
 World* world = nullptr;
 entity::Entity* camera = nullptr;
 float tickDelta = 0.0f;
 WorldRenderStage stage = WorldRenderStage::Sky;
 RenderHookMoment moment = RenderHookMoment::Before;
 bool cancelVanilla = false;
 bool vanillaStageRan = false;
 float celestialAngle = 0.0f;
 float skyYawDegrees = 0.0f;
 float starBrightness = 0.0f;
 float rainStrength = 0.0f;
 bool starsEnabled = true;
 bool astronomyEnabled = false;
 double astronomyUtcMillis = 0.0;
 float observerLatitudeDegrees = 0.0f;
 float observerLongitudeDegrees = 0.0f;
 bool solarDirectionValid = false;
 float sunDirectionX = 0.0f;
 float sunDirectionY = 1.0f;
 float sunDirectionZ = 0.0f;
 float sunAzimuthDegrees = 0.0f;
 float sunAltitudeDegrees = 90.0f;
 bool shadowPass = false;
};
struct FirstPersonHandRenderEvent {
 entity::LivingEntity* camera = nullptr;
 float tickDelta = 0.0f;
 int eye = 0;
 bool canceled = false;
};
enum class WorldColorKind {
 Sky,
 Fog,
};
struct WorldColorEvent {
 const World* world = nullptr;
 entity::Entity* entity = nullptr;
 float partialTicks = 0.0f;
 Vec3d color;
 WorldColorKind kind = WorldColorKind::Sky;
};
struct FogSettingsEvent {
 World* world = nullptr;
 entity::Entity* camera = nullptr;
 bool enabled = false;
 bool spherical = true;
 bool exponential = false;
 float start = 0.2f;
 float end = 0.8f;
 float density = 0.1f;
 bool customColor = false;
 float red = 0.0f;
 float green = 0.0f;
 float blue = 0.0f;
};
struct ModelPartPose {
 float yaw = std::numeric_limits<float>::quiet_NaN();
 float pitch = std::numeric_limits<float>::quiet_NaN();
 float roll = std::numeric_limits<float>::quiet_NaN();
};
struct EntityRenderPose {
 float bodyYaw = 0.0f;
 float headYaw = 0.0f;
 float headPitch = 0.0f;
 float limbSwing = 0.0f;
 float limbDistance = 0.0f;
 float yaw = 0.0f;
 float pitch = 0.0f;
 float roll = 0.0f;
 float scale = 1.0f;
 float offsetX = 0.0f;
 float offsetY = 0.0f;
 float offsetZ = 0.0f;
 std::unordered_map<std::string, ModelPartPose> parts;
};
struct PreEntityRenderEvent {
 const entity::Entity* entity = nullptr;
 int entityId = 0;
 int entityRawId = 0;
 std::string entityType = "unknown";
 float tickDelta = 0.0f;
 bool canceled = false;
};
struct EntityRenderEvent {
 const entity::LivingEntity* entity = nullptr;
 int entityId = 0;
 std::string entityType = "unknown";
 bool isPlayer = false;
 float tickDelta = 0.0f;
 EntityRenderPose pose;
};
enum class LifecyclePhase {
 NotStarted,
 Init,
 PostInit,
 Ready
};
struct LifecycleEvent {
 LifecyclePhase previous = LifecyclePhase::NotStarted;
 LifecyclePhase current = LifecyclePhase::NotStarted;
};
} // namespace net::minecraft::mod
namespace net::minecraft::mod::runtime {
template <typename Fill, typename Read>
void dispatchLuaHook(int eventIndex, Fill fill, Read read);
[[nodiscard]] bool hasLuaHook(int eventIndex);
[[nodiscard]] inline bool hasLuaHook(LuaEventId id) {
 return hasLuaHook(static_cast<int>(id));
}
void invalidateLuaHookCache();
[[nodiscard]] bool isSupportedLuaEvent(std::string_view event);
void subscribeLuaCallback(const std::shared_ptr<ModHost::LoadedLuaMod>& mod,
                          const ModHost::LoadedLuaMod::Callback& callback);
using LifecycleListener = std::function<void(LifecyclePhase, LifecyclePhase)>;
void registerLifecycleListener(int order, LifecycleListener listener);
void fireLifecycle(LifecyclePhase previous, LifecyclePhase current);
using ChunkStageListener =
    std::function<void(world::gen::ChunkGenerationEvent&)>;
void registerChunkStageListener(world::gen::ChunkStage stage, int priority, ChunkStageListener listener);
void fireChunkGeneration(world::gen::ChunkGenerationEvent& event);
void luaHookClientTick(ClientTickEvent& event);
void luaHookRenderFrame(RenderFrameEvent& event);
void luaHookFirstPersonHand(FirstPersonHandRenderEvent& event);
void luaHookKeyPress(KeyPressEvent& event);
void luaHookMouseButton(MouseButtonEvent& event);
void luaHookRaycast(RaycastEvent& event);
void luaHookFov(FovEvent& event);
void luaHookCameraSetup(CameraSetupEvent& event);
void luaHookPlayerTravel(PlayerTravelEvent& event);
void luaHookTickRate(TickRateEvent& event);
void luaHookWorldStart(WorldStartEvent& event);
void luaHookWorldOpen(WorldOpenEvent& event);
void luaHookWorldTick(WorldTickEvent& event);
void luaHookEntityTick(EntityTickEvent& event);
void luaHookTileEntityTick(TileEntityTickEvent& event);
void luaHookCreateWorld(CreateWorldEvent& event);
void luaHookBlockInteract(BlockInteractEvent& event);
void luaHookEntityInteract(EntityInteractEvent& event);
void luaHookAttackDamage(AttackDamageEvent& event);
void luaHookEntityTeleport(EntityTeleportEvent& event);
void luaHookWorldColor(WorldColorEvent& event);
void luaHookFogSettings(FogSettingsEvent& event);
void luaHookEntityRender(EntityRenderEvent& event);
void luaHookWorldRender(WorldRenderEvent& event);
void luaHookChunkGeneration(world::gen::ChunkGenerationEvent& event);
void luaHookScreenRegion(ScreenRegionEvent& event);
void luaHookWorldSpawnSearch(WorldSpawnSearchEvent& event);
void luaHookScreenUi(ScreenUiEvent& event);
#ifdef MINECRAFT_NATIVE_EXPORTS
void luaHookScreenEvent(LuaScreenEvent& event);
#endif
void luaHookPreEntityRender(PreEntityRenderEvent& event);
void luaHookPreTileEntityRender(PreTileEntityRenderEvent& event);
void luaHookEntitySpawn(EntitySpawnEvent& event);
void luaHookEntityRemove(EntityRemoveEvent& event);
} // namespace net::minecraft::mod::runtime
