#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include <limits>
#include <string>
#include <unordered_map>
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
namespace net::minecraft::mod {
struct CameraSetupEvent {
  entity::LivingEntity* camera = nullptr;
  float tickDelta = 0.0f;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  bool customView = false;
  bool hideFirstPersonHand = false;
};
struct RenderTargetsEvent {
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
  Clouds,
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
// Per-entity render-time pose override. Render-only: never touches simulation.
// Rotations are degrees; limb_swing is the walk phase (radians), limb_distance
// the walk amplitude (0..1); scale is a multiplier; offsets are world units.
// parts[x] are model-part rotations in degrees (NaN = leave as-is).
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
struct EntityRenderEvent {
  const entity::LivingEntity* entity = nullptr;
  int entityId = 0;
  std::string entityType = "unknown";
  bool isPlayer = false;
  float tickDelta = 0.0f;
  EntityRenderPose pose;
};
} // namespace net::minecraft::mod
