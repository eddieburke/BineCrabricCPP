#include "net/minecraft/mod/runtime/LuaRaycastBindings.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/InteractionManager.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#endif
#include <cmath>
#include <optional>
#include <string>
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
using namespace net::minecraft::util::math;
namespace {
#ifdef MINECRAFT_NATIVE_EXPORTS
[[nodiscard]] std::optional<HitResult> boxRaycast(const Box& box, const Vec3d& start, const Vec3d& end) {
 Vec3d dir{end.x - start.x, end.y - start.y, end.z - start.z};
 double tMin = 0.0;
 double tMax = 1.0;
 for(int axis = 0; axis < 3; ++axis) {
  const double s = axis == 0 ? start.x : (axis == 1 ? start.y : start.z);
  const double d = axis == 0 ? dir.x : (axis == 1 ? dir.y : dir.z);
  const double bMin = axis == 0 ? box.minX : (axis == 1 ? box.minY : box.minZ);
  const double bMax = axis == 0 ? box.maxX : (axis == 1 ? box.maxY : box.maxZ);
  if(std::abs(d) < 1.0e-7) {
   if(s < bMin || s > bMax) {
    return std::nullopt;
   }
   continue;
  }
  double t1 = (bMin - s) / d;
  double t2 = (bMax - s) / d;
  if(t1 > t2) {
   std::swap(t1, t2);
  }
  tMin = std::max(tMin, t1);
  tMax = std::min(tMax, t2);
  if(tMin > tMax) {
   return std::nullopt;
  }
 }
 const double tHit = tMin >= 0.0 ? tMin : tMax;
 if(tHit < 0.0 || tHit > 1.0) {
  return std::nullopt;
 }
 return HitResult{MathHelper::floor(start.x + dir.x * tHit),
                  MathHelper::floor(start.y + dir.y * tHit),
                  MathHelper::floor(start.z + dir.z * tHit),
                  0,
                  {start.x + dir.x * tHit, start.y + dir.y * tHit, start.z + dir.z * tHit}};
}
[[nodiscard]] std::optional<HitResult> entityRaycast(
    World* world, Entity* except, const Vec3d& start, const Vec3d& end, double maxDistance) {
 Vec3d dir{end.x - start.x, end.y - start.y, end.z - start.z};
 const double length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
 if(length < 1.0e-7 || world == nullptr) {
  return std::nullopt;
 }
 Box query = except != nullptr ? except->boundingBox.stretch(dir.x, dir.y, dir.z).expand(1.0)
                               : Box{start.x - maxDistance,
                                     start.y - maxDistance,
                                     start.z - maxDistance,
                                     start.x + maxDistance,
                                     start.y + maxDistance,
                                     start.z + maxDistance};
 const std::vector<Entity*> candidates = world->getEntities(except, query);
 Entity* closest = nullptr;
 Vec3d closestPos{};
 double closestDist = maxDistance;
 for(Entity* entity : candidates) {
  if(entity == nullptr || !entity->isCollidable()) {
   continue;
  }
  const Box hitBox = entity->boundingBox.expand(static_cast<double>(entity->getTargetingMargin()));
  const std::optional<HitResult> hit = boxRaycast(hitBox, start, end);
  if(!hit.has_value()) {
   continue;
  }
  const double dist = std::sqrt((hit->pos.x - start.x) * (hit->pos.x - start.x) +
                                (hit->pos.y - start.y) * (hit->pos.y - start.y) +
                                (hit->pos.z - start.z) * (hit->pos.z - start.z));
  if(dist < closestDist) {
   closest = entity;
   closestPos = hit->pos;
   closestDist = dist;
  }
 }
 if(closest == nullptr) {
  return std::nullopt;
 }
 return HitResult(closest, closestPos);
}
void pushRaycastResult(lua_State* state, const std::optional<HitResult>& hit, World* world) {
 LuaApi& api = luaApi();
 if(!hit.has_value()) {
  api.pushnil(state);
  return;
 }
 api.createtable(state, 0, 16);
 setField(state, "type", hit->type == HitResultType::ENTITY ? "entity" : "block");
 setField(state, "hit_x", hit->pos.x);
 setField(state, "hit_y", hit->pos.y);
 setField(state, "hit_z", hit->pos.z);
 if(hit->type == HitResultType::BLOCK) {
  setField(state, "block_x", hit->blockX);
  setField(state, "block_y", hit->blockY);
  setField(state, "block_z", hit->blockZ);
  setField(state, "side", hit->side);
  const int blockId = world != nullptr ? world->getBlockId(hit->blockX, hit->blockY, hit->blockZ) : 0;
  setField(state, "block_id", blockId);
  setField(state, "block_name", blockWireNameFromId(blockId));
  setField(state, "item_id", blockId);
 } else if(hit->type == HitResultType::ENTITY && hit->entity != nullptr) {
  setEntityIdentityFields(state, *hit->entity);
  setField(state, "entity_raw_id", entity::EntityRegistry::getRawId(*hit->entity));
  const Vec3d p = hit->entity->position();
  setField(state, "entity_x", p.x);
  setField(state, "entity_y", p.y);
  setField(state, "entity_z", p.z);
 }
}
[[nodiscard]] bool readRaycastOrigin(lua_State* state,
                                     int tableIndex,
                                     Vec3d& origin,
                                     Vec3d& direction,
                                     double& maxDistance,
                                     bool& ignoreLiquids,
                                     bool& doBlocks,
                                     bool& doEntities) {
 LuaApi& api = luaApi();
 if(api.type(state, tableIndex) != kLuaTTable) {
  return false;
 }
 if(api.getfield(state, tableIndex, "direction") == kLuaTTable) {
  const int dirIndex = api.gettop(state);
  direction.x = luaFloatAt(state, dirIndex, 1, luaFloatField(state, dirIndex, "x", 0.0));
  direction.y = luaFloatAt(state, dirIndex, 2, luaFloatField(state, dirIndex, "y", 0.0));
  direction.z = luaFloatAt(state, dirIndex, 3, luaFloatField(state, dirIndex, "z", 0.0));
  api.settop(state, tableIndex);
 } else {
  api.settop(state, tableIndex);
  if(api.getfield(state, tableIndex, "yaw") != kLuaTNil ||
     api.getfield(state, tableIndex, "pitch") != kLuaTNil) {
   api.settop(state, tableIndex);
   const double yaw = luaFloatField(state, tableIndex, "yaw", 0.0);
   const double pitch = luaFloatField(state, tableIndex, "pitch", 0.0);
   const double radYaw = yaw / 180.0 * kPiF;
   const double radPitch = pitch / 180.0 * kPiF;
   const double cosPitch = std::cos(radPitch);
   direction.x = -std::sin(radYaw) * cosPitch;
   direction.y = -std::sin(radPitch);
   direction.z = std::cos(radYaw) * cosPitch;
  } else {
   api.settop(state, tableIndex);
  }
 }
 if(api.getfield(state, tableIndex, "origin") == kLuaTTable) {
  const int originIndex = api.gettop(state);
  origin.x = luaFloatAt(state, originIndex, 1, luaFloatField(state, originIndex, "x", 0.0));
  origin.y = luaFloatAt(state, originIndex, 2, luaFloatField(state, originIndex, "y", 0.0));
  origin.z = luaFloatAt(state, originIndex, 3, luaFloatField(state, originIndex, "z", 0.0));
  api.settop(state, tableIndex);
 } else {
  api.settop(state, tableIndex);
 }
 if(api.getfield(state, tableIndex, "origin_x") != kLuaTNil) {
  api.settop(state, tableIndex);
  origin.x = luaFloatField(state, tableIndex, "origin_x", origin.x);
  origin.y = luaFloatField(state, tableIndex, "origin_y", origin.y);
  origin.z = luaFloatField(state, tableIndex, "origin_z", origin.z);
 } else {
  api.settop(state, tableIndex);
 }
 maxDistance = luaFloatField(state, tableIndex, "max_distance", luaFloatField(state, tableIndex, "reach", 0.0));
 ignoreLiquids = luaBoolField(state, tableIndex, "ignore_liquids", false);
 doBlocks = luaBoolField(state, tableIndex, "blocks", true);
 doEntities = luaBoolField(state, tableIndex, "entities", true);
 return true;
}
#endif
} // namespace
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaRaycast(lua_State* state) {
 LuaApi& api = luaApi();
 Vec3d origin{};
 Vec3d direction{};
 double maxDistance = 0.0;
 bool ignoreLiquids = false;
 bool doBlocks = true;
 bool doEntities = true;
 bool explicitRay = false;
 if(api.gettop(state) >= 1 && api.type(state, 1) == kLuaTTable) {
  explicitRay = readRaycastOrigin(state, 1, origin, direction, maxDistance, ignoreLiquids, doBlocks, doEntities);
 }
 client::Minecraft* client = client::Minecraft::INSTANCE;
 if(client == nullptr || client->world == nullptr) {
  api.pushnil(state);
  return 1;
 }
 World* world = client->world;
 Entity* camera =
     client->camera != nullptr ? static_cast<Entity*>(client->camera) : static_cast<Entity*>(client->player);
 if(camera == nullptr) {
  api.pushnil(state);
  return 1;
 }
 if(!explicitRay || (direction.x == 0.0 && direction.y == 0.0 && direction.z == 0.0)) {
  if(auto* living = dynamic_cast<entity::LivingEntity*>(camera)) {
   origin = living->getPosition(1.0f);
   direction = living->getLookVector(1.0f);
  } else {
   origin = camera->position();
   direction = camera->getLookVector();
  }
  if(maxDistance <= 0.0 && client->interactionManager != nullptr) {
   maxDistance = static_cast<double>(client->interactionManager->getReachDistance());
  }
 }
 if(maxDistance <= 0.0) {
  maxDistance = 5.0;
 }
 const double dirLength =
     std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
 if(dirLength < 1.0e-7) {
  api.pushnil(state);
  return 1;
 }
 direction.x /= dirLength;
 direction.y /= dirLength;
 direction.z /= dirLength;
 const Vec3d end{origin.x + direction.x * maxDistance,
                 origin.y + direction.y * maxDistance,
                 origin.z + direction.z * maxDistance};
 std::optional<HitResult> blockHit;
 double blockDist = maxDistance;
 if(doBlocks) {
  blockHit = world->raycast(origin, end, ignoreLiquids);
  if(blockHit.has_value()) {
   blockDist = std::sqrt((blockHit->pos.x - origin.x) * (blockHit->pos.x - origin.x) +
                         (blockHit->pos.y - origin.y) * (blockHit->pos.y - origin.y) +
                         (blockHit->pos.z - origin.z) * (blockHit->pos.z - origin.z));
  }
 }
 std::optional<HitResult> entityHit;
 if(doEntities) {
  entityHit = entityRaycast(world, camera, origin, end, blockDist);
 }
 // Placed model instances share the entity-style hitbox path: they win when
 // their box is nearer than any block hit along the ray.
 model::ModelRaycastHit modelHit;
 const bool haveModel = model::raycastModelInstances(
     origin.x, origin.y, origin.z, direction.x, direction.y, direction.z, blockDist, modelHit);
 if(haveModel) {
  api.createtable(state, 0, 8);
  setField(state, "type", "model");
  setField(state, "model_id", modelHit.instanceId);
  setField(state, "model_tag", modelHit.tag);
  setField(state, "hit_x", modelHit.x);
  setField(state, "hit_y", modelHit.y);
  setField(state, "hit_z", modelHit.z);
  setField(state, "distance", modelHit.distance);
  return 1;
 }
 if(entityHit.has_value() && (!blockHit.has_value() || blockHit->type != HitResultType::BLOCK)) {
  pushRaycastResult(state, entityHit, world);
  return 1;
 }
 pushRaycastResult(state, blockHit, world);
 return 1;
}
#else
int luaRaycast(lua_State* state) {
 luaApi().pushnil(state);
 return 1;
}
#endif
void installRaycastApi(lua_State* state) {
 bindFunctions(state, {{"raycast", luaRaycast}});
}
} // namespace net::minecraft::mod::runtime
