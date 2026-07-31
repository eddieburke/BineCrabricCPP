#include "net/minecraft/mod/runtime/LuaEntityBindings.hpp"
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#endif
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
struct PoseHookRegistration {
 std::shared_ptr<ModHost::LoadedLuaMod> mod;
 int ref = 0;
};
// entities.* are reachable both as minecraft.entities.get(id) and as
// handle:get(id); the latter passes the table as argument 1.
int selfArgOffset(lua_State* state) {
 return luaApi().type(state, 1) == kLuaTTable ? 1 : 0;
}
// The shared_ptr the host holds for the mod running this call. Pose hooks keep
// the mod alive for as long as the hook is registered.
std::shared_ptr<ModHost::LoadedLuaMod> currentLuaModShared(lua_State* state) {
 const ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr) {
  return nullptr;
 }
 for(const auto& loaded : loadedLuaMods()) {
  if(loaded.get() == mod) {
   return loaded;
  }
 }
 return nullptr;
}
std::unordered_map<std::string, std::vector<PoseHookRegistration>>& globalPoseHooks() {
 static std::unordered_map<std::string, std::vector<PoseHookRegistration>> hooks;
 return hooks;
}
std::unordered_map<int, std::vector<PoseHookRegistration>>& localPoseHooks() {
 static std::unordered_map<int, std::vector<PoseHookRegistration>> hooks;
 return hooks;
}
void pushPosePart(lua_State* state, const net::minecraft::mod::ModelPartPose& part) {
 luaApi().createtable(state, 0, 3);
 setField(state, "yaw", part.yaw);
 setField(state, "pitch", part.pitch);
 setField(state, "roll", part.roll);
}
void pushPoseTable(lua_State* state, const net::minecraft::mod::EntityRenderPose& pose) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 13);
 setField(state, "body_yaw", pose.bodyYaw);
 setField(state, "head_yaw", pose.headYaw);
 setField(state, "head_pitch", pose.headPitch);
 setField(state, "limb_swing", pose.limbSwing);
 setField(state, "limb_distance", pose.limbDistance);
 setField(state, "yaw", pose.yaw);
 setField(state, "pitch", pose.pitch);
 setField(state, "roll", pose.roll);
 setField(state, "scale", pose.scale);
 setField(state, "offset_x", pose.offsetX);
 setField(state, "offset_y", pose.offsetY);
 setField(state, "offset_z", pose.offsetZ);
 api.createtable(state, 0, static_cast<int>(pose.parts.size()));
 for(const auto& [name, part] : pose.parts) {
  pushPosePart(state, part);
  api.setfield(state, -2, name.c_str());
 }
 api.setfield(state, -2, "parts");
}
void readPosePart(lua_State* state, int tableIndex, net::minecraft::mod::ModelPartPose& part) {
 part.yaw = luaFloatField(state, tableIndex, "yaw", part.yaw);
 part.pitch = luaFloatField(state, tableIndex, "pitch", part.pitch);
 part.roll = luaFloatField(state, tableIndex, "roll", part.roll);
}
void applyPoseTable(lua_State* state, int tableIndex, net::minecraft::mod::EntityRenderPose& pose) {
 LuaApi& api = luaApi();
 pose.bodyYaw = luaFloatField(state, tableIndex, "body_yaw", pose.bodyYaw);
 pose.headYaw = luaFloatField(state, tableIndex, "head_yaw", pose.headYaw);
 pose.headPitch = luaFloatField(state, tableIndex, "head_pitch", pose.headPitch);
 pose.limbSwing = luaFloatField(state, tableIndex, "limb_swing", pose.limbSwing);
 pose.limbDistance = luaFloatField(state, tableIndex, "limb_distance", pose.limbDistance);
 pose.yaw = luaFloatField(state, tableIndex, "yaw", pose.yaw);
 pose.pitch = luaFloatField(state, tableIndex, "pitch", pose.pitch);
 pose.roll = luaFloatField(state, tableIndex, "roll", pose.roll);
 pose.scale = luaFloatField(state, tableIndex, "scale", pose.scale);
 pose.offsetX = luaFloatField(state, tableIndex, "offset_x", pose.offsetX);
 pose.offsetY = luaFloatField(state, tableIndex, "offset_y", pose.offsetY);
 pose.offsetZ = luaFloatField(state, tableIndex, "offset_z", pose.offsetZ);
 api.getfield(state, tableIndex, "parts");
 if(api.type(state, -1) == kLuaTTable) {
  api.pushnil(state);
  while(api.next(state, -2) != 0) {
   const std::string name = api.type(state, -2) == kLuaTString ? luaString(state, -2, "") : std::string();
   if(!name.empty() && api.type(state, -1) == kLuaTTable) {
    readPosePart(state, api.gettop(state), pose.parts[name]);
   }
   pop(state, 1);
  }
 }
 pop(state, 1);
}
void applyPoseHook(const PoseHookRegistration& hook,
                   const net::minecraft::LivingEntity& entity,
                   float tickDelta,
                   net::minecraft::mod::EntityRenderPose& pose) {
 if(hook.mod == nullptr || hook.ref == 0) {
  return;
 }
 callLuaEvent(
     hook.mod,
     hook.ref,
     [&entity, &pose, tickDelta](lua_State* state) {
      setField(state, "entity_id", entity.id);
      setField(state, "entity_type", net::minecraft::entity::EntityRegistry::getId(entity));
      setField(state, "tick_delta", tickDelta);
      pushPoseTable(state, pose);
      luaApi().setfield(state, -2, "pose");
     },
     [&pose](lua_State* state) {
      luaApi().getfield(state, -1, "pose");
      if(luaApi().type(state, -1) == kLuaTTable) {
       applyPoseTable(state, luaApi().gettop(state), pose);
      }
      pop(state, 1);
     });
}
// Shared by both pose-hook registrars: validate (key, function), take a registry
// ref to the callback, and hand back the owning mod. Returns nullptr on reject.
std::shared_ptr<ModHost::LoadedLuaMod> takePoseHookRef(lua_State* state, int keyType, int& refOut) {
 LuaApi& api = luaApi();
 if(api.gettop(state) < 2 || api.type(state, 1) != keyType || api.type(state, 2) != kLuaTFunction) {
  return nullptr;
 }
 std::shared_ptr<ModHost::LoadedLuaMod> owner = currentLuaModShared(state);
 if(owner == nullptr) {
  return nullptr;
 }
 api.pushvalue(state, 2);
 refOut = api.ref(state, kLuaRegistryIndex);
 return owner;
}
int luaRegisterGlobalPoseHook(lua_State* state) {
 LuaApi& api = luaApi();
 int ref = 0;
 std::shared_ptr<ModHost::LoadedLuaMod> owner = takePoseHookRef(state, kLuaTString, ref);
 if(owner == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 globalPoseHooks()[luaString(state, 1, "")].push_back({std::move(owner), ref});
 api.pushboolean(state, 1);
 return 1;
}
int luaRegisterLocalPoseHook(lua_State* state) {
 LuaApi& api = luaApi();
 int ref = 0;
 std::shared_ptr<ModHost::LoadedLuaMod> owner = takePoseHookRef(state, kLuaTNumber, ref);
 if(owner == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 localPoseHooks()[luaIntArg(state, 1)].push_back({std::move(owner), ref});
 api.pushboolean(state, 1);
 return 1;
}
int luaUnregisterLocalPoseHook(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.gettop(state) < 1 || api.type(state, 1) != kLuaTNumber) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int entityId = luaIntArg(state, 1);
 api.pushboolean(state, localPoseHooks().erase(entityId) > 0 ? 1 : 0);
 return 1;
}
// A world holds a flat entity list, so an id lookup is a scan either way. The
// map this used to build cost one allocation-heavy pass per call — including on
// entities.get, which then used exactly one entry of it.
net::minecraft::entity::Entity* findEntity(World* world, int id) {
 if(world == nullptr) {
  return nullptr;
 }
 for(net::minecraft::entity::Entity* e : world->entities()) {
  if(e != nullptr && e->id == id) {
   return e;
  }
 }
 return nullptr;
}
// The handle passed to a mod-entity mutator has to name a LuaModEntity this mod
// registered, and the client may only touch its own client-local replicas.
net::minecraft::mod::lua::LuaModEntity* ownedModEntity(lua_State* state, World* world, int id) {
 auto* entity = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(findEntity(world, id));
 if(entity == nullptr || (world->isRemote() && !entity->isClientLocal())) {
  return nullptr;
 }
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || !entity->registryId().starts_with(mod->modId + ":")) {
  return nullptr;
 }
 return entity;
}
void applySingleEntityState(lua_State* state, net::minecraft::mod::lua::LuaModEntity* e, int entryIndex) {
 LuaApi& api = luaApi();
 if(api.getfield(state, entryIndex, "x") == kLuaTNumber) {
  e->setPosition(api.tonumberx(state, -1, nullptr),
                 luaDoubleField(state, entryIndex, "y", e->y),
                 luaDoubleField(state, entryIndex, "z", e->z));
 }
 pop(state, 1);
 if(api.getfield(state, entryIndex, "vx") == kLuaTNumber) {
  e->velocityX = api.tonumberx(state, -1, nullptr);
  e->velocityY = luaDoubleField(state, entryIndex, "vy", e->velocityY);
  e->velocityZ = luaDoubleField(state, entryIndex, "vz", e->velocityZ);
 }
 pop(state, 1);
 if(api.getfield(state, entryIndex, "yaw") == kLuaTNumber) {
  e->yaw = static_cast<float>(api.tonumberx(state, -1, nullptr));
 }
 pop(state, 1);
 if(api.getfield(state, entryIndex, "pitch") == kLuaTNumber) {
  e->pitch = static_cast<float>(api.tonumberx(state, -1, nullptr));
 }
 pop(state, 1);
 if(api.getfield(state, entryIndex, "data") == kLuaTTable) {
  e->setData(NbtCompound(luaValueToNbt(state, api.gettop(state))));
 }
 pop(state, 1);
}
int luaEntitiesApplyState(lua_State* state) {
 LuaApi& api = luaApi();
 World* world = activeModWorld();
 if(world == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTTable ||
    api.type(state, 2) != kLuaTTable) {
  api.pushboolean(state, 0);
  return 1;
 }
 auto* entity = ownedModEntity(state, world, luaIntField(state, 1, "id", -1));
 if(entity == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 applySingleEntityState(state, entity, 2);
 api.pushboolean(state, 1);
 return 1;
}
int luaEntitiesTeleport(lua_State* state) {
 LuaApi& api = luaApi();
 World* world = activeModWorld();
 if(world == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTTable) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int id = luaIntField(state, 1, "id", -1);
 // The server may move any entity; a client only its own client-local replicas.
 net::minecraft::entity::Entity* e =
     world->isRemote() ? ownedModEntity(state, world, id) : findEntity(world, id);
 if(e == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 double x = e->x;
 double y = e->y;
 double z = e->z;
 float yaw = e->yaw;
 float pitch = e->pitch;
 if(api.type(state, 2) == kLuaTTable) {
  const int posIndex = 2;
  x = luaDoubleField(state, posIndex, "x", e->x);
  y = luaDoubleField(state, posIndex, "y", e->y);
  z = luaDoubleField(state, posIndex, "z", e->z);
  yaw = luaFloatField(state, posIndex, "yaw", e->yaw);
  pitch = luaFloatField(state, posIndex, "pitch", e->pitch);
 } else {
  if(api.gettop(state) >= 4 && api.type(state, 2) == kLuaTNumber) {
   x = luaDoubleArg(state, 2);
   y = luaDoubleArg(state, 3);
   z = luaDoubleArg(state, 4);
  }
  if(api.gettop(state) >= 6 && api.type(state, 5) == kLuaTNumber) {
   yaw = luaFloatArg(state, 5);
   pitch = luaFloatArg(state, 6);
  }
 }
 e->teleport(x, y, z, yaw, pitch);
 api.pushboolean(state, 1);
 return 1;
}
int luaEntitiesRemove(lua_State* state) {
 LuaApi& api = luaApi();
 World* world = activeModWorld();
 if(world == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
  api.pushboolean(state, 0);
  return 1;
 }
 auto* e = ownedModEntity(state, world, luaIntField(state, 1, "id", -1));
 if(e == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 e->markDead();
 api.pushboolean(state, 1);
 return 1;
}
void pushEntityHandle(lua_State* state, net::minecraft::entity::Entity* e) {
 LuaApi& api = luaApi();
 if(e == nullptr) {
  api.pushnil(state);
  return;
 }
 const std::string type = net::minecraft::entity::EntityRegistry::getId(*e);
 const auto* modEntity = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(e);
 const std::string registryId = modEntity != nullptr ? modEntity->registryId() : std::string();
 api.createtable(state, 0, 22);
 setField(state, "id", e->id);
 setField(state, "type", type);
 setField(state, "fire_ticks", e->fireTicks);
 setField(state, "is_on_fire", e->isOnFire());
 if(type == "Player") {
  auto* player = static_cast<net::minecraft::entity::player::PlayerEntity*>(e);
  const ItemStack* held = player->inventory.getSelectedItem();
  if(held != nullptr && !held->empty()) {
   setField(state, "held_item_id", held->itemId);
  } else {
   setField(state, "held_item_id", 0);
  }
 }
 if(modEntity != nullptr) {
  setField(state, "registry_id", registryId);
  setField(state, "client_local", modEntity->isClientLocal());
  pushNbtValue(state, modEntity->data().storage());
  api.setfield(state, -2, "data");
 }
 setField(state, "x", e->x);
 setField(state, "y", e->y);
 setField(state, "z", e->z);
 setField(state, "vx", e->velocityX);
 setField(state, "vy", e->velocityY);
 setField(state, "vz", e->velocityZ);
 setField(state, "yaw", e->yaw);
 setField(state, "pitch", e->pitch);
 setField(state, "on_ground", e->onGround);
 if(type == "Item") {
  auto* item = static_cast<net::minecraft::entity::ItemEntity*>(e);
  pushItemStackFields(state, item->stack);
 }
 bindFunctions(state,
               {
                   {"teleport", luaEntitiesTeleport},
                   {"apply_state", luaEntitiesApplyState},
                   {"remove", luaEntitiesRemove},
               });
}
int luaEntitiesList(lua_State* state) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 16);
 World* world = activeModWorld();
 if(world == nullptr) {
  return 1;
 }
 const int argOffset = selfArgOffset(state);
 const std::string filter = api.gettop(state) >= 1 + argOffset && api.type(state, 1 + argOffset) == kLuaTString
                                ? luaString(state, 1 + argOffset, "")
                                : std::string();
 int count = 0;
 for(net::minecraft::entity::Entity* e : world->entities()) {
  if(e == nullptr) {
   continue;
  }
  const std::string type = net::minecraft::entity::EntityRegistry::getId(*e);
  const auto* modEntity = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(e);
  const std::string registryId = modEntity != nullptr ? modEntity->registryId() : std::string();
  if(!filter.empty() && type != filter && registryId != filter) {
   continue;
  }
  pushEntityHandle(state, e);
  api.rawseti(state, -2, ++count);
 }
 return 1;
}
int luaEntitiesGet(lua_State* state) {
 LuaApi& api = luaApi();
 World* world = activeModWorld();
 if(world == nullptr || api.gettop(state) < 1) {
  api.pushnil(state);
  return 1;
 }
 // pushEntityHandle(nil) already pushes nil, so a miss needs no special case.
 pushEntityHandle(state, findEntity(world, luaIntArg(state, 1 + selfArgOffset(state))));
 return 1;
}
int luaEntitiesSpawnMod(lua_State* state) {
 LuaApi& api = luaApi();
 World* world = activeModWorld();
 if(world == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString ||
    api.type(state, 2) != kLuaTTable) {
  api.pushnil(state);
  return 1;
 }
 const std::string registryId = luaString(state, 1, "");
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 const std::size_t separator = registryId.find(':');
 if(registryId.empty() || mod == nullptr || separator == std::string::npos ||
    registryId.substr(0, separator) != mod->modId) {
  api.pushnil(state);
  return 1;
 }
 const int specIndex = 2;
 const double x = luaDoubleField(state, specIndex, "x", 0.0);
 const double y = luaDoubleField(state, specIndex, "y", 0.0);
 const double z = luaDoubleField(state, specIndex, "z", 0.0);
 const float yaw = luaFloatField(state, specIndex, "yaw", 0.0f);
 const float pitch = luaFloatField(state, specIndex, "pitch", 0.0f);
 // The client may only spawn replicas it owns outright; everything else is the
 // server's to create and replicate.
 const bool clientLocal = luaBoolField(state, specIndex, "client_local", false);
 if(world->isRemote() && !clientLocal) {
  api.pushnil(state);
  return 1;
 }
 auto* modEntity = new net::minecraft::mod::lua::LuaModEntity(world);
 modEntity->setRegistryId(registryId);
 modEntity->setClientLocal(clientLocal);
 if(api.getfield(state, specIndex, "data") == kLuaTTable) {
  modEntity->setData(NbtCompound(luaValueToNbt(state, api.gettop(state))));
 }
 pop(state, 1);
 modEntity->setPositionAndAngles(x, y, z, yaw, pitch);
 if(world->spawnEntity(modEntity)) {
  pushEntityHandle(state, modEntity);
 } else {
  delete modEntity;
  api.pushnil(state);
 }
 return 1;
}
} // namespace
void clearLocalPoseHook(int entityId) {
 localPoseHooks().erase(entityId);
}
void applyRegisteredPoseHooks(const net::minecraft::LivingEntity& entity,
                              float tickDelta,
                              net::minecraft::mod::EntityRenderPose& pose) {
 if(globalPoseHooks().empty() && localPoseHooks().empty()) {
  return;
 }
 const std::string& entityType = net::minecraft::entity::EntityRegistry::getIdRef(entity);
 if(const auto it = globalPoseHooks().find(entityType); it != globalPoseHooks().end()) {
  for(const PoseHookRegistration& hook : it->second) {
   applyPoseHook(hook, entity, tickDelta, pose);
  }
 }
 if(const auto it = localPoseHooks().find(entity.id); it != localPoseHooks().end()) {
  for(const PoseHookRegistration& hook : it->second) {
   applyPoseHook(hook, entity, tickDelta, pose);
  }
 }
}
void installEntityApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 LuaApi& api = luaApi();
 pushFunctionTable(state,
                   {
                       {"list", luaEntitiesList},
                       {"get", luaEntitiesGet},
                       {"apply_state", luaEntitiesApplyState},
                       {"teleport", luaEntitiesTeleport},
                       {"remove", luaEntitiesRemove},
                       {"unregister_local_pose_hook", luaUnregisterLocalPoseHook},
                   });
 bindModFunction(state, &mod, "spawn_mod", luaEntitiesSpawnMod);
 bindModFunction(state, &mod, "register_global_pose_hook", luaRegisterGlobalPoseHook);
 bindModFunction(state, &mod, "register_local_pose_hook", luaRegisterLocalPoseHook);
 api.setfield(state, -2, "entities");
}
} // namespace net::minecraft::mod::runtime
