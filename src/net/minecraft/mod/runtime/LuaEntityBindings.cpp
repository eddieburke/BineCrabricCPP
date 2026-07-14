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
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
struct PoseHookRegistration {
  std::shared_ptr<ModHost::LoadedLuaMod> mod;
  int ref = 0;
};
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
  api.createtable(state, 0, 11);
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
int luaRegisterGlobalPoseHook(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString ||
     api.type(state, 2) != kLuaTFunction) {
    api.pushboolean(state, 0);
    return 1;
  }
  std::shared_ptr<ModHost::LoadedLuaMod> owner;
  for(const auto& loaded : loadedLuaMods()) {
    if(loaded.get() == mod) {
      owner = loaded;
      break;
    }
  }
  if(owner == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string entityType = luaString(state, 1, "");
  api.pushvalue(state, 2);
  const int ref = api.ref(state, kLuaRegistryIndex);
  globalPoseHooks()[entityType].push_back({owner, ref});
  api.pushboolean(state, 1);
  return 1;
}
int luaRegisterLocalPoseHook(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTNumber ||
     api.type(state, 2) != kLuaTFunction) {
    api.pushboolean(state, 0);
    return 1;
  }
  std::shared_ptr<ModHost::LoadedLuaMod> owner;
  for(const auto& loaded : loadedLuaMods()) {
    if(loaded.get() == mod) {
      owner = loaded;
      break;
    }
  }
  if(owner == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int entityId = luaIntArg(state, 1);
  api.pushvalue(state, 2);
  const int ref = api.ref(state, kLuaRegistryIndex);
  localPoseHooks()[entityId].push_back({owner, ref});
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
World* entityWorld() {
  return contextOrClientWorld();
}
bool canMutateEntities(const World* world) {
  return world != nullptr && !world->isRemote();
}
std::unordered_map<int, net::minecraft::entity::Entity*> buildIndex(World* world) {
  std::unordered_map<int, net::minecraft::entity::Entity*> index;
  if(world != nullptr) {
    for(net::minecraft::entity::Entity* e : world->entities()) {
      if(e != nullptr) {
        index[e->id] = e;
      }
    }
  }
  return index;
}
void applySingleEntityState(lua_State* state, int id, int entryIndex, const std::unordered_map<int, net::minecraft::entity::Entity*>& index) {
  LuaApi& api = luaApi();
  auto it = index.find(id);
  if(it == index.end()) {
    return;
  }
  auto* e = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(it->second);
  if(e == nullptr) {
    return;
  }
  if(api.getfield(state, entryIndex, "x") == kLuaTNumber) {
    int isNumber = 0;
    const double x = api.tonumberx(state, -1, &isNumber);
    const double y = luaFloatField(state, entryIndex, "y", static_cast<float>(e->y));
    const double z = luaFloatField(state, entryIndex, "z", static_cast<float>(e->z));
    e->setPosition(x, y, z);
  }
  pop(state, 1);
  if(api.getfield(state, entryIndex, "vx") == kLuaTNumber) {
    int isNumber = 0;
    const double vx = api.tonumberx(state, -1, &isNumber);
    const double vy = luaFloatField(state, entryIndex, "vy", static_cast<float>(e->velocityY));
    const double vz = luaFloatField(state, entryIndex, "vz", static_cast<float>(e->velocityZ));
    e->velocityX = vx;
    e->velocityY = vy;
    e->velocityZ = vz;
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
  World* world = entityWorld();
  if(!canMutateEntities(world) || api.gettop(state) < 2 || api.type(state, 1) != kLuaTTable || api.type(state, 2) != kLuaTTable) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int id = luaIntField(state, 1, "id", -1);
  auto index = buildIndex(world);
  applySingleEntityState(state, id, 2, index);
  api.pushboolean(state, 1);
  return 1;
}
int luaEntitiesTeleport(lua_State* state) {
  LuaApi& api = luaApi();
  World* world = entityWorld();
  if(!canMutateEntities(world) || api.gettop(state) < 2 || api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int id = luaIntField(state, 1, "id", -1);
  auto index = buildIndex(world);
  auto it = index.find(id);
  if(it == index.end() || it->second == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  net::minecraft::entity::Entity* e = it->second;
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
  World* world = entityWorld();
  if(!canMutateEntities(world) || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int id = luaIntField(state, 1, "id", -1);
  auto index = buildIndex(world);
  auto it = index.find(id);
  if(it == index.end()) {
    api.pushboolean(state, 0);
    return 1;
  }
  auto* e = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(it->second);
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
  api.createtable(state, 0, 18);
  setField(state, "id", e->id);
  setField(state, "type", type);
  if(modEntity != nullptr) {
    setField(state, "registry_id", registryId);
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
    const ItemStack& stack = item->stack;
    setField(state, "item_id", stack.itemId);
    setField(state, "item_count", stack.count);
    setField(state, "item_damage", stack.damage);
    setField(state, "item_max_damage", stack.getMaxDamage());
#ifdef MINECRAFT_NATIVE_EXPORTS
    const bool modTex = net::minecraft::client::render::item::ItemModelRenderer::usesModTexture(stack);
    if(modTex) {
      const auto* spec = itemRegistrationSpecForId(stack.itemId);
      const std::string path = spec != nullptr ? spec->texturePath : std::string();
      setField(state, "texture_path", path);
      setField(state, "mod_texture", true);
      setField(state, "atlas_index", -1);
    } else {
      setField(state,
               "texture_path",
               net::minecraft::client::render::item::ItemModelRenderer::spriteAtlasPath(stack));
      setField(state, "mod_texture", false);
      setField(state, "atlas_index", stack.getTextureId());
    }
#endif
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
  World* world = entityWorld();
  if(world == nullptr) {
    return 1;
  }
  int argOffset = 0;
  if(api.type(state, 1) == kLuaTTable) {
    argOffset = 1;
  }
  const std::string filter =
      api.gettop(state) >= 1 + argOffset && api.type(state, 1 + argOffset) == kLuaTString ? luaString(state, 1 + argOffset, "") : std::string();
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
  World* world = entityWorld();
  if(world == nullptr || api.gettop(state) < 1) {
    api.pushnil(state);
    return 1;
  }
  int argOffset = 0;
  if(api.type(state, 1) == kLuaTTable) {
    argOffset = 1;
  }
  const int id = luaIntArg(state, 1 + argOffset);
  auto index = buildIndex(world);
  auto it = index.find(id);
  if(it == index.end()) {
    api.pushnil(state);
    return 1;
  }
  pushEntityHandle(state, it->second);
  return 1;
}
int luaEntitiesSpawnMod(lua_State* state) {
  LuaApi& api = luaApi();
  World* world = entityWorld();
  if(!canMutateEntities(world) || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString ||
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
  auto* modEntity = new net::minecraft::mod::lua::LuaModEntity(world);
  modEntity->setRegistryId(registryId);
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
void applyRegisteredPoseHooks(const net::minecraft::LivingEntity& entity,
                              float tickDelta,
                              net::minecraft::mod::EntityRenderPose& pose) {
  const std::string entityType = net::minecraft::entity::EntityRegistry::getId(entity);
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
