#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/mod/runtime/LuaBlockEntityBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
// ModHostUtil.hpp deleted — its functions now live in LuaHostApi.hpp and others
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#endif
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
template <typename Event>
World* eventWorld(Event& event) {
 if constexpr(requires { static_cast<World*>(event.world); }) {
  if(event.world != nullptr) {
   return event.world;
  }
 }
 if constexpr(requires { event.entity->world; }) {
  if(event.entity != nullptr) {
   return event.entity->world;
  }
 }
 if constexpr(requires { event.player->world; }) {
  if(event.player != nullptr) {
   return event.player->world;
  }
 }
 if constexpr(requires { event.target->world; }) {
  if(event.target != nullptr) {
   return event.target->world;
  }
 }
 return nullptr;
}
template <typename Event>
entity::player::PlayerEntity* eventPlayer(Event& event) {
 if constexpr(requires { static_cast<entity::player::PlayerEntity*>(event.player); }) {
  return event.player;
 }
 return nullptr;
}
constexpr auto kNoRead = [](lua_State*, auto&) {};
[[maybe_unused]] [[nodiscard]] ChunkWriteMode chunkWriteModeForStage(world::gen::ChunkStage stage) {
 switch(stage) {
 case world::gen::ChunkStage::Terrain:
 case world::gen::ChunkStage::Surface:
 case world::gen::ChunkStage::Carver:
  return ChunkWriteMode::RawGeneration;
 case world::gen::ChunkStage::Features:
  return ChunkWriteMode::ChunkApi;
 }
 return ChunkWriteMode::ChunkApi;
}
[[nodiscard]] const char* chunkStageName(world::gen::ChunkStage stage) {
 switch(stage) {
 case world::gen::ChunkStage::Terrain:
  return "terrain";
 case world::gen::ChunkStage::Surface:
  return "surface";
 case world::gen::ChunkStage::Carver:
  return "carver";
 case world::gen::ChunkStage::Features:
  return "features";
 }
 return "unknown";
}
#ifdef MINECRAFT_NATIVE_EXPORTS
class ModDrawScope {
 public:
 ModDrawScope() : saved_(client::render::RenderSystem::getShadow()) {
 }
 ~ModDrawScope() {
  client::render::RenderSystem::setShadow(saved_);
 }
 ModDrawScope(const ModDrawScope&) = delete;
 ModDrawScope& operator=(const ModDrawScope&) = delete;

 private:
 client::render::RenderSystem::StateShadow saved_;
};
[[nodiscard]] const char* renderStageName(WorldRenderStage stage) {
 switch(stage) {
 case WorldRenderStage::Sky:
  return "sky";
 case WorldRenderStage::Stars:
  return "stars";
 case WorldRenderStage::OpaqueTerrain:
  return "terrain_opaque";
 case WorldRenderStage::Entities:
  return "entities";
 case WorldRenderStage::LitParticles:
  return "particles_lit";
 case WorldRenderStage::Particles:
  return "particles";
 case WorldRenderStage::TranslucentTerrain:
  return "terrain_translucent";
 case WorldRenderStage::Weather:
  return "weather";
 case WorldRenderStage::Clouds:
  return "clouds";
 case WorldRenderStage::Hand:
  return "hand";
 case WorldRenderStage::Framebuffer:
  return "framebuffer";
 }
 return "unknown";
}
[[nodiscard]] const char* renderMomentName(RenderHookMoment moment) {
 return moment == RenderHookMoment::Before ? "before" : "after";
}
void pushItemEntityFields(lua_State* state, const net::minecraft::entity::Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 const auto* item = dynamic_cast<const net::minecraft::entity::ItemEntity*>(entity);
 if(item == nullptr) {
  return;
 }
 const ItemStack& stack = item->stack;
 setFields(state, "item_id", stack.itemId, "item_count", stack.count, "item_damage", stack.damage);
 const bool modTex = net::minecraft::client::render::item::ItemModelRenderer::usesModTexture(stack);
 if(modTex) {
  const auto* spec = itemRegistrationSpecForId(stack.itemId);
  const std::string path = spec != nullptr ? spec->texturePath : std::string();
  setFields(state, "texture_path", path, "mod_texture", true, "atlas_index", -1);
 } else {
  setFields(state,
            "texture_path",
            net::minecraft::client::render::item::ItemModelRenderer::spriteAtlasPath(stack),
            "mod_texture",
            false,
            "atlas_index",
            stack.getTextureId());
 }
}
#endif
void setChunkContextFields(lua_State* state) {
 setFields(state,
           "chunk_x",
           LuaChunkContext::activeChunkX(),
           "chunk_z",
           LuaChunkContext::activeChunkZ(),
           "has_chunk",
           LuaChunkContext::hasActiveChunk());
}
// Common shape: scope on the event world, fill + read the Lua table.
template <typename Event, typename Push, typename Read>
void runLuaHook(LuaEventId id, Event& e, Push push, Read read) {
 const int eventIndex = static_cast<int>(id);
 if(!hasLuaHook(eventIndex)) {
  return;
 }
 World* world = eventWorld(e);
 auto fill = [&e, &push, world](lua_State* state) {
  push(state, e);
  setLuaExecutionFields(state, world);
 };
 auto apply = [&e, &read](lua_State* state) { read(state, e); };
 if(world != nullptr) {
  ModContextScope scope(world, eventPlayer(e));
  dispatchLuaHook(eventIndex, fill, apply);
 } else {
  dispatchLuaHook(eventIndex, fill, apply);
 }
}
} // namespace
namespace {
std::array<std::uint16_t, kLuaEventCount> gHookCounts{};
bool gHookCountsValid = false;
void rebuildHookCounts() {
 gHookCounts.fill(0);
 for(const auto& mod : loadedLuaMods()) {
  if(mod == nullptr || !mod->active) {
   continue;
  }
  for(const auto& cb : mod->callbacks) {
   if(cb.eventIndex >= 0 && static_cast<std::size_t>(cb.eventIndex) < kLuaEventCount) {
    ++gHookCounts[static_cast<std::size_t>(cb.eventIndex)];
   }
  }
 }
 gHookCountsValid = true;
}
} // namespace
void invalidateLuaHookCache() {
 gHookCountsValid = false;
}
bool hasLuaHook(int eventIndex) {
 if(eventIndex < 0 || static_cast<std::size_t>(eventIndex) >= kLuaEventCount) {
  return false;
 }
 if(!gHookCountsValid) {
  rebuildHookCounts();
 }
 return gHookCounts[static_cast<std::size_t>(eventIndex)] != 0;
}
template <typename Fill, typename Read>
void dispatchLuaHook(int eventIndex, Fill fill, Read read) {
 if(!isLuaModExecutionEnabled()) {
  return;
 }
 for(const auto& mod : loadedLuaMods()) {
  if(mod == nullptr || !mod->active) {
   continue;
  }
  for(const auto& cb : mod->callbacks) {
   if(cb.eventIndex != eventIndex) {
    continue;
   }
   callLuaEvent(mod, cb.functionRef, fill, read);
  }
 }
}
[[nodiscard]] bool isSupportedLuaEvent(std::string_view event) {
 return luaEventIndexOf(event) >= 0;
}
void subscribeLuaCallback(const std::shared_ptr<ModHost::LoadedLuaMod>&, const ModHost::LoadedLuaMod::Callback&) {
 invalidateLuaHookCache();
}
namespace {
struct LifecycleListenerEntry {
 int order = 0;
 LifecycleListener listener;
};
std::vector<LifecycleListenerEntry>& lifecycleListeners() {
 static std::vector<LifecycleListenerEntry> value;
 return value;
}
struct ChunkStageListenerEntry {
 int priority = 0;
 ChunkStageListener listener;
};
std::array<std::vector<ChunkStageListenerEntry>, 4>& chunkStageListeners() {
 static std::array<std::vector<ChunkStageListenerEntry>, 4> value{};
 return value;
}
std::size_t chunkStageIndex(world::gen::ChunkStage stage) {
 return static_cast<std::size_t>(stage);
}
} // namespace
void registerLifecycleListener(int order, LifecycleListener listener) {
 lifecycleListeners().push_back({order, std::move(listener)});
 std::stable_sort(
     lifecycleListeners().begin(),
     lifecycleListeners().end(),
     [](const LifecycleListenerEntry& a, const LifecycleListenerEntry& b) { return a.order < b.order; });
}
void fireLifecycle(LifecyclePhase previous, LifecyclePhase current) {
 for(const auto& entry : lifecycleListeners()) {
  entry.listener(previous, current);
 }
}
void registerChunkStageListener(world::gen::ChunkStage stage, int priority, ChunkStageListener listener) {
 chunkStageListeners()[chunkStageIndex(stage)].push_back({priority, std::move(listener)});
}
void fireChunkGeneration(world::gen::ChunkGenerationEvent& event) {
 for(const auto& entry : chunkStageListeners()[chunkStageIndex(event.stage)]) {
  entry.listener(event);
 }
 if(hasLuaHook(LuaEventId::ChunkGeneration)) {
  luaHookChunkGeneration(event);
 }
}
void luaHookClientTick(ClientTickEvent& e) {
 runLuaHook(
     LuaEventId::ClientTick,
     e,
     [](lua_State* state, ClientTickEvent& ev) { setClientTickFields(state, ev); },
     kNoRead);
}
void luaHookRenderFrame(RenderFrameEvent& e) {
 runLuaHook(
     LuaEventId::RenderFrame,
     e,
     [](lua_State* state, RenderFrameEvent& ev) { setFields(state, "tick_delta", ev.tickDelta); },
     kNoRead);
}
void luaHookFirstPersonHand(FirstPersonHandRenderEvent& e) {
 runLuaHook(
     LuaEventId::FirstPersonHand,
     e,
     [](lua_State* state, FirstPersonHandRenderEvent& ev) {
      setFields(state, "tick_delta", ev.tickDelta, "eye", ev.eye, "canceled", ev.canceled);
      if(ev.camera != nullptr) {
       setEntityIdentityFields(state, *ev.camera);
      }
     },
     [](lua_State* state, FirstPersonHandRenderEvent& ev) { readField(state, "canceled", ev.canceled); });
}
void luaHookKeyPress(KeyPressEvent& e) {
 if(e.pressed && !e.repeat) {
  net::minecraft::mod::ModSettingsRegistry::instance().markKeyPressed(e.key);
 }
 runLuaHook(
     LuaEventId::KeyPress,
     e,
     [](lua_State* state, KeyPressEvent& ev) {
      setFields(state, "key", ev.key, "pressed", ev.pressed, "repeat", ev.repeat, "handled", ev.handled);
     },
     [](lua_State* state, KeyPressEvent& ev) { readField(state, "handled", ev.handled); });
}
void luaHookMouseButton(MouseButtonEvent& e) {
 runLuaHook(
     LuaEventId::MouseButton,
     e,
     [](lua_State* state, MouseButtonEvent& ev) {
      setFields(state, "button", ev.button, "pressed", ev.pressed, "handled", ev.handled);
     },
     [](lua_State* state, MouseButtonEvent& ev) { readField(state, "handled", ev.handled); });
}
void luaHookRaycast(RaycastEvent& e) {
 runLuaHook(
     LuaEventId::Raycast,
     e,
     [](lua_State* state, RaycastEvent& ev) {
      const char* typeName = "none";
      if(ev.hasHit) {
       typeName = ev.type == HitResultType::ENTITY ? "entity" : "block";
      }
      setFields(state,
                "has_hit",
                ev.hasHit,
                "type",
                typeName,
                "hit_x",
                ev.hitX,
                "hit_y",
                ev.hitY,
                "hit_z",
                ev.hitZ,
                "block_x",
                ev.blockX,
                "block_y",
                ev.blockY,
                "block_z",
                ev.blockZ,
                "side",
                ev.side,
                "block_id",
                ev.blockId,
                "block_name",
                blockWireNameFromId(ev.blockId),
                "item_id",
                ev.blockId);
      if(ev.entity != nullptr) {
       const auto pos = ev.entity->position();
       setEntityIdentityFields(state, *ev.entity);
       setFields(state,
                 "entity_raw_id",
                 entity::EntityRegistry::getRawId(*ev.entity),
                 "entity_x",
                 pos.x,
                 "entity_y",
                 pos.y,
                 "entity_z",
                 pos.z);
      }
     },
     kNoRead);
}
void luaHookFov(FovEvent& e) {
 runLuaHook(
     LuaEventId::Fov,
     e,
     [](lua_State* state, FovEvent& ev) { setFields(state, "tick_delta", ev.tickDelta, "fov", ev.fov); },
     [](lua_State* state, FovEvent& ev) { readField(state, "fov", ev.fov); });
}
void luaHookCameraSetup(CameraSetupEvent& e) {
 runLuaHook(
     LuaEventId::CameraSetup,
     e,
     [](lua_State* state, CameraSetupEvent& ev) {
      setFields(state,
                "tick_delta",
                ev.tickDelta,
                "x",
                ev.x,
                "y",
                ev.y,
                "z",
                ev.z,
                "yaw",
                ev.yaw,
                "pitch",
                ev.pitch,
                "roll",
                ev.roll,
                "custom_view",
                ev.customView,
                "hide_first_person_hand",
                ev.hideFirstPersonHand);
     },
     [](lua_State* state, CameraSetupEvent& ev) {
      readFields(state,
                 "x",
                 ev.x,
                 "y",
                 ev.y,
                 "z",
                 ev.z,
                 "yaw",
                 ev.yaw,
                 "pitch",
                 ev.pitch,
                 "roll",
                 ev.roll,
                 "custom_view",
                 ev.customView,
                 "hide_first_person_hand",
                 ev.hideFirstPersonHand);
     });
}
void luaHookPlayerTravel(PlayerTravelEvent& e) {
 runLuaHook(
     LuaEventId::PlayerTravel,
     e,
     [](lua_State* state, PlayerTravelEvent& ev) {
      setFields(state,
                "sideways",
                ev.sideways,
                "forward",
                ev.forward,
                "speed_multiplier",
                ev.speedMultiplier,
                "has_player",
                ev.player != nullptr);
#ifdef MINECRAFT_NATIVE_EXPORTS
      const client::Minecraft* client = client::Minecraft::INSTANCE;
      const void* player = static_cast<const void*>(ev.player);
      const bool isLocal = player != nullptr && client != nullptr &&
                           (static_cast<const void*>(client->player) == player ||
                            static_cast<const void*>(client->camera) == player);
      setField(state, "is_local_player", isLocal);
#else
      setField(state, "is_local_player", false);
#endif
     },
     [](lua_State* state, PlayerTravelEvent& ev) {
      readFields(state, "sideways", ev.sideways, "forward", ev.forward, "speed_multiplier", ev.speedMultiplier);
     });
}
void luaHookTickRate(TickRateEvent& e) {
 runLuaHook(
     LuaEventId::TickRate,
     e,
     [](lua_State* state, TickRateEvent& ev) {
      setFields(state, "target_tps", ev.targetTps, "tps_scale", ev.tpsScale);
     },
     [](lua_State* state, TickRateEvent& ev) {
      readFields(state, "target_tps", ev.targetTps, "tps_scale", ev.tpsScale);
     });
}
void luaHookWorldStart(WorldStartEvent& e) {
 runLuaHook(
     LuaEventId::WorldStart,
     e,
     [](lua_State* state, WorldStartEvent& ev) {
      setFields(
          state, "save_name", ev.saveName != nullptr ? *ev.saveName : std::string(), "new_world", ev.newWorld);
     },
     kNoRead);
}
void luaHookWorldOpen(WorldOpenEvent& e) {
 runLuaHook(
     LuaEventId::WorldOpen,
     e,
     [](lua_State* state, WorldOpenEvent& ev) {
      setFields(
          state, "save_name", ev.saveName != nullptr ? *ev.saveName : std::string(), "new_world", ev.newWorld);
      if(ev.options != nullptr) {
       pushStringMap(state, *ev.options);
      } else {
       pushStringMap(state, {});
      }
      luaApi().setfield(state, -2, "options");
     },
     kNoRead);
}
void luaHookWorldTick(WorldTickEvent& e) {
 runLuaHook(
     LuaEventId::WorldTick,
     e,
     [](lua_State* state, WorldTickEvent& ev) { setFields(state, "remote", ev.remote, "before", ev.before); },
     kNoRead);
}
void luaHookEntityTick(EntityTickEvent& e) {
 runLuaHook(
     LuaEventId::EntityTick,
     e,
     [](lua_State* state, EntityTickEvent& ev) {
      setFields(state, "remote", ev.remote, "canceled", ev.canceled);
      if(ev.entity != nullptr) {
       setEntityIdentityFields(state, *ev.entity);
       setFields(state,
                 "x",
                 ev.entity->x,
                 "y",
                 ev.entity->y,
                 "z",
                 ev.entity->z,
                 "yaw",
                 ev.entity->yaw,
                 "pitch",
                 ev.entity->pitch);
      }
     },
     [](lua_State* state, EntityTickEvent& ev) { readField(state, "canceled", ev.canceled); });
}
void luaHookTileEntityTick(TileEntityTickEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(e.entity == nullptr) {
  return;
 }
 ModContextScope context(e.world);
 if(!e.animationTicked) {
  tickTileEntityAnimation(e.entity);
  e.animationTicked = true;
 }
 dispatchLuaHook(
     static_cast<int>(LuaEventId::TileEntityTick),
     [&e](lua_State* state) {
      setFields(state,
                "x",
                e.entity->x,
                "y",
                e.entity->y,
                "z",
                e.entity->z,
                "id",
                e.entity->id(),
                "remote",
                e.remote,
                "removed",
                e.entity->isRemoved(),
                "canceled",
                e.canceled,
                "world_time",
                e.entity->world != nullptr ? static_cast<double>(e.entity->world->getTime()) : 0.0,
                "animation_frame",
                tileEntityAnimationFrame(e.entity),
                "animation_tick",
                static_cast<double>(tileEntityAnimTick(e.entity)),
                "animation_speed",
                tileEntityAnimationSpeed(e.entity));
      pushTileEntityHandle(state, e.entity);
      luaApi().setfield(state, -2, "entity");
     },
     [&e](lua_State* state) {
      readField(state, "canceled", e.canceled);
      setTileEntityAnimationSpeed(
          e.entity, luaFloatField(state, -1, "animation_speed", tileEntityAnimationSpeed(e.entity)));
     });
#else
 (void)e;
#endif
}
void luaHookCreateWorld(CreateWorldEvent& e) {
 runLuaHook(
     LuaEventId::CreateWorld,
     e,
     [](lua_State* state, CreateWorldEvent& ev) {
      setFields(state,
                "save_name",
                ev.saveName != nullptr ? *ev.saveName : std::string(),
                "seed",
                ev.seed,
                "canceled",
                ev.canceled);
      pushStringMap(state, ev.options);
      luaApi().setfield(state, -2, "options");
     },
     [](lua_State* state, CreateWorldEvent& ev) {
      readField(state, "canceled", ev.canceled);
      readStringMapField(state, -1, "options", ev.options);
     });
}
void luaHookBlockInteract(BlockInteractEvent& e) {
 runLuaHook(
     LuaEventId::BlockInteract,
     e,
     [](lua_State* state, BlockInteractEvent& ev) {
      setFields(state,
                "x",
                ev.x,
                "y",
                ev.y,
                "z",
                ev.z,
                "block_id",
                getBlockIdAt(ev.world, ev.x, ev.y, ev.z),
                "side",
                ev.side,
                "right_click",
                ev.rightClick,
                "remote",
                ev.world != nullptr && ev.world->isRemote(),
                "canceled",
                ev.canceled,
                "handled",
                ev.handled,
                "has_player",
                ev.player != nullptr,
                "local_player",
                isLocalPlayer(ev.player),
                "has_item",
                ev.stack != nullptr && !ev.stack->empty());
      if(ev.player != nullptr) {
       setFields(state,
                 "player_x",
                 ev.player->x,
                 "player_y",
                 ev.player->y,
                 "player_z",
                 ev.player->z,
                 "player_yaw",
                 ev.player->yaw,
                 "player_pitch",
                 ev.player->pitch);
      }
      if(ev.stack != nullptr && !ev.stack->empty()) {
       setFields(state,
                 "item_id",
                 ev.stack->itemId,
                 "item_count",
                 ev.stack->count,
                 "item_damage",
                 ev.stack->damage,
                 "item_max_damage",
                 ev.stack->getMaxDamage(),
                 "item_damageable",
                 ev.stack->isDamageable());
      }
     },
     [](lua_State* state, BlockInteractEvent& ev) {
      readFields(state, "canceled", ev.canceled, "handled", ev.handled);
      if(ev.stack != nullptr && !ev.stack->empty()) {
       ev.stack->count = std::max(0, luaIntField(state, -1, "item_count", ev.stack->count));
       if(ev.stack->isDamageable()) {
        ev.stack->damage = std::clamp(
            luaIntField(state, -1, "item_damage", ev.stack->damage), 0, ev.stack->getMaxDamage());
       }
      }
     });
}
void luaHookEntityInteract(EntityInteractEvent& e) {
 runLuaHook(
     LuaEventId::EntityInteract,
     e,
     [](lua_State* state, EntityInteractEvent& ev) {
      setFields(state,
                "attack",
                ev.attack,
                "remote",
                ev.player != nullptr && ev.player->world != nullptr && ev.player->world->isRemote(),
                "canceled",
                ev.canceled,
                "handled",
                ev.handled,
                "sneaking",
                ev.sneaking,
                "has_player",
                ev.player != nullptr,
                "local_player",
                isLocalPlayer(ev.player),
                "has_target",
                ev.target != nullptr);
      if(ev.player != nullptr) {
       setFields(state, "player_yaw", ev.player->yaw, "player_pitch", ev.player->pitch);
      }
      if(ev.stack != nullptr && !ev.stack->empty()) {
       setFields(state,
                 "has_item",
                 true,
                 "item_id",
                 ev.stack->itemId,
                 "item_count",
                 ev.stack->count,
                 "item_damage",
                 ev.stack->damage);
      } else {
       setField(state, "has_item", false);
      }
      if(ev.target != nullptr) {
       setEntityIdentityFields(state, *ev.target);
       setField(state, "target_id", ev.target->id);
      }
     },
     [](lua_State* state, EntityInteractEvent& ev) {
      readFields(state, "canceled", ev.canceled, "handled", ev.handled);
      if(ev.stack != nullptr && !ev.stack->empty()) {
       ev.stack->count = std::max(0, luaIntField(state, -1, "item_count", ev.stack->count));
      }
     });
}
void luaHookAttackDamage(AttackDamageEvent& e) {
 runLuaHook(
     LuaEventId::AttackDamage,
     e,
     [](lua_State* state, AttackDamageEvent& ev) {
      setFields(state,
                "damage",
                ev.damage,
                "critical",
                ev.critical,
                "canceled",
                ev.canceled,
                "fall_distance",
                ev.fallDistance,
                "on_ground",
                ev.onGround,
                "target_x",
                ev.targetX,
                "target_y",
                ev.targetY,
                "target_z",
                ev.targetZ,
                "has_player",
                ev.player != nullptr,
                "has_target",
                ev.target != nullptr);
     },
     [](lua_State* state, AttackDamageEvent& ev) {
      readFields(state, "damage", ev.damage, "critical", ev.critical, "canceled", ev.canceled);
     });
}
void luaHookEntityTeleport(EntityTeleportEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(e.entity != nullptr && e.entity->world != nullptr && e.entity->world->isRemote()) {
  return;
 }
 ModContextScope context(e.world, dynamic_cast<net::minecraft::entity::player::PlayerEntity*>(e.entity));
 dispatchLuaHook(
     static_cast<int>(LuaEventId::EntityTeleport),
     [&e](lua_State* state) {
      setFields(state,
                "entity_id",
                e.entity != nullptr ? e.entity->id : -1,
                "entity_type",
                e.entity != nullptr ? net::minecraft::entity::EntityRegistry::getId(*e.entity) : std::string(),
                "from_x",
                e.fromX,
                "from_y",
                e.fromY,
                "from_z",
                e.fromZ,
                "x",
                e.x,
                "y",
                e.y,
                "z",
                e.z,
                "yaw",
                e.yaw,
                "pitch",
                e.pitch,
                "canceled",
                e.canceled,
                "has_entity",
                e.entity != nullptr,
                "has_player",
                dynamic_cast<net::minecraft::entity::player::PlayerEntity*>(e.entity) != nullptr);
      setWorldContextFields(state, e.world);
     },
     [&e](lua_State* state) {
      readFields(state, "x", e.x, "y", e.y, "z", e.z, "yaw", e.yaw, "pitch", e.pitch, "canceled", e.canceled);
     });
#else
 (void)e;
#endif
}
void luaHookWorldColor(WorldColorEvent& e) {
 runLuaHook(
     LuaEventId::WorldColor,
     e,
     [](lua_State* state, WorldColorEvent& ev) {
      setFields(state,
                "partial_ticks",
                ev.partialTicks,
                "r",
                ev.color.x,
                "g",
                ev.color.y,
                "b",
                ev.color.z,
                "kind",
                ev.kind == WorldColorKind::Sky ? "sky" : "fog");
      setWorldContextFields(state, ev.world);
#ifdef MINECRAFT_NATIVE_EXPORTS
      if(ev.world != nullptr) {
       setFields(state,
                 "celestial",
                 static_cast<double>(normalizedCelestial(ev.world, ev.partialTicks)),
                 "world_time",
                 static_cast<double>(ev.world->getTime() % 24000ULL),
                 "is_night",
                 worldIsNight(ev.world));
      }
#endif
     },
     [](lua_State* state, WorldColorEvent& ev) {
      const auto component = [state](const char* name, double fallback) {
       const float value = luaFloatField(state, -1, name, static_cast<float>(fallback));
       return std::isfinite(value) ? static_cast<double>(std::clamp(value, 0.0f, 1.0f)) : fallback;
      };
      ev.color.x = component("r", ev.color.x);
      ev.color.y = component("g", ev.color.y);
      ev.color.z = component("b", ev.color.z);
     });
}
void luaHookFogSettings(FogSettingsEvent& e) {
 runLuaHook(
     LuaEventId::FogSettings,
     e,
     [](lua_State* state, FogSettingsEvent& ev) {
      setFields(state,
                "enabled",
                ev.enabled,
                "spherical",
                ev.spherical,
                "exponential",
                ev.exponential,
                "start",
                ev.start,
                "end",
                ev.end,
                "density",
                ev.density,
                "custom_color",
                ev.customColor,
                "red",
                ev.red,
                "green",
                ev.green,
                "blue",
                ev.blue);
      setWorldContextFields(state, ev.world);
     },
     [](lua_State* state, FogSettingsEvent& ev) {
      readFields(state,
                 "enabled",
                 ev.enabled,
                 "spherical",
                 ev.spherical,
                 "exponential",
                 ev.exponential,
                 "custom_color",
                 ev.customColor);
      ev.start = std::clamp(luaFloatField(state, -1, "start", ev.start), 0.0f, 1.0f);
      ev.end = std::clamp(luaFloatField(state, -1, "end", ev.end), 0.0f, 1.0f);
      ev.density = std::clamp(luaFloatField(state, -1, "density", ev.density), 0.0f, 1.0f);
      ev.red = std::clamp(luaFloatField(state, -1, "red", ev.red), 0.0f, 1.0f);
      ev.green = std::clamp(luaFloatField(state, -1, "green", ev.green), 0.0f, 1.0f);
      ev.blue = std::clamp(luaFloatField(state, -1, "blue", ev.blue), 0.0f, 1.0f);
     });
}
void luaHookEntityRender(EntityRenderEvent& e) {
 runLuaHook(
     LuaEventId::EntityRender,
     e,
     [](lua_State* state, EntityRenderEvent& ev) {
      setFields(state,
                "entity_id",
                ev.entityId,
                "entity_type",
                ev.entityType,
                "is_player",
                ev.isPlayer,
                "tick_delta",
                ev.tickDelta);
      luaApi().createtable(state, 0, 12);
      setFields(state,
                "body_yaw",
                ev.pose.bodyYaw,
                "head_yaw",
                ev.pose.headYaw,
                "head_pitch",
                ev.pose.headPitch,
                "limb_swing",
                ev.pose.limbSwing,
                "limb_distance",
                ev.pose.limbDistance,
                "yaw",
                ev.pose.yaw,
                "pitch",
                ev.pose.pitch,
                "roll",
                ev.pose.roll,
                "scale",
                ev.pose.scale,
                "offset_x",
                ev.pose.offsetX,
                "offset_y",
                ev.pose.offsetY,
                "offset_z",
                ev.pose.offsetZ);
      luaApi().createtable(state, 0, static_cast<int>(ev.pose.parts.size()));
      for(const auto& [name, part] : ev.pose.parts) {
       luaApi().createtable(state, 0, 3);
       setFields(state, "yaw", part.yaw, "pitch", part.pitch, "roll", part.roll);
       luaApi().setfield(state, -2, name.c_str());
      }
      luaApi().setfield(state, -2, "parts");
      luaApi().setfield(state, -2, "pose");
     },
     [](lua_State* state, EntityRenderEvent& ev) {
      luaApi().getfield(state, -1, "pose");
      if(luaApi().type(state, -1) == kLuaTTable) {
       readFields(state,
                  "body_yaw",
                  ev.pose.bodyYaw,
                  "head_yaw",
                  ev.pose.headYaw,
                  "head_pitch",
                  ev.pose.headPitch,
                  "limb_swing",
                  ev.pose.limbSwing,
                  "limb_distance",
                  ev.pose.limbDistance,
                  "yaw",
                  ev.pose.yaw,
                  "pitch",
                  ev.pose.pitch,
                  "roll",
                  ev.pose.roll,
                  "scale",
                  ev.pose.scale,
                  "offset_x",
                  ev.pose.offsetX,
                  "offset_y",
                  ev.pose.offsetY,
                  "offset_z",
                  ev.pose.offsetZ);
       luaApi().getfield(state, -1, "parts");
       if(luaApi().type(state, -1) == kLuaTTable) {
        luaApi().pushnil(state);
        while(luaApi().next(state, -2) != 0) {
         if(luaApi().type(state, -2) == kLuaTString && luaApi().type(state, -1) == kLuaTTable) {
          const std::string name = luaString(state, -2, "");
          if(!name.empty()) {
           ModelPartPose& part = ev.pose.parts[name];
           readFields(state, "yaw", part.yaw, "pitch", part.pitch, "roll", part.roll);
          }
         }
         pop(state, 1);
        }
       }
       pop(state, 1);
      }
      pop(state, 1);
     });
}
void luaHookWorldRender(WorldRenderEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(!hasLuaHook(LuaEventId::WorldRender)) {
  return;
 }
 ScopedModWorldDrawContext worldDrawScope{e.world, e.tickDelta};
 const ModDrawScope modCaps;
 dispatchLuaHook(
     static_cast<int>(LuaEventId::WorldRender),
     [&e](lua_State* state) {
      setFields(state,
                "tick_delta",
                e.tickDelta,
                "stage",
                renderStageName(e.stage),
                "moment",
                renderMomentName(e.moment),
                "cancel_vanilla",
                e.cancelVanilla,
                "vanilla_stage_ran",
                e.vanillaStageRan,
                "shadow_pass",
                e.shadowPass,
                "celestial_angle",
                static_cast<double>(e.celestialAngle),
                "sky_yaw_deg",
                static_cast<double>(e.skyYawDegrees),
                "star_brightness",
                static_cast<double>(e.starBrightness),
                "rain_strength",
                static_cast<double>(e.rainStrength),
                "stars_enabled",
                e.starsEnabled,
                "astronomy_enabled",
                e.astronomyEnabled,
                "astronomy_utc_millis",
                e.astronomyUtcMillis,
                "observer_latitude_deg",
                static_cast<double>(e.observerLatitudeDegrees),
                "observer_longitude_deg",
                static_cast<double>(e.observerLongitudeDegrees));
      if(e.solarDirectionValid) {
       setFields(state,
                 "sun_direction_x",
                 e.sunDirectionX,
                 "sun_direction_y",
                 e.sunDirectionY,
                 "sun_direction_z",
                 e.sunDirectionZ,
                 "sun_azimuth_deg",
                 e.sunAzimuthDegrees,
                 "sun_altitude_deg",
                 e.sunAltitudeDegrees);
      }
      setWorldContextFields(state, e.world);
      const client::render::FrameRenderCamera& frameCamera =
          client::render::RenderCameraState::instance().frame();
      const double cameraX = frameCamera.x;
      const double cameraY = frameCamera.y;
      const double cameraZ = frameCamera.z;
#ifdef MINECRAFT_NATIVE_EXPORTS
      if(e.world != nullptr) {
       setFields(state,
                 "world_time",
                 static_cast<double>(e.world->getTime() % 24000ULL),
                 "celestial",
                 static_cast<double>(normalizedCelestial(e.world, e.tickDelta)),
                 "is_night",
                 worldIsNight(e.world));
      }
      if(e.stage == WorldRenderStage::Clouds && e.world != nullptr && e.world->dimension != nullptr) {
       float cloudBaseHeight = e.world->dimension->getCloudHeight() - static_cast<float>(cameraY) + 0.33f;
       if(client::Minecraft* client = client::Minecraft::INSTANCE; client != nullptr) {
        cloudBaseHeight =
            client::option::cloudHeightOffset(cloudBaseHeight, client::option::resolve(client->options));
       }
       setField(state, "cloud_base_height", cloudBaseHeight);
      }
#endif
      setFields(state,
                "camera_x",
                cameraX,
                "camera_y",
                cameraY,
                "camera_z",
                cameraZ,
                "eye_x",
                frameCamera.eyeX,
                "eye_y",
                frameCamera.eyeY,
                "eye_z",
                frameCamera.eyeZ,
                "camera_yaw",
                static_cast<double>(frameCamera.yaw),
                "camera_pitch",
                static_cast<double>(frameCamera.pitch),
                "camera_roll",
                static_cast<double>(frameCamera.roll),
                "custom_camera",
                frameCamera.customView);
     },
     [&e](lua_State* state) {
      readField(state, "cancel_vanilla", e.cancelVanilla);
      if(e.stage == WorldRenderStage::Sky && e.moment == RenderHookMoment::Before) {
       readFields(state,
                  "celestial_angle",
                  e.celestialAngle,
                  "sky_yaw_deg",
                  e.skyYawDegrees,
                  "astronomy_enabled",
                  e.astronomyEnabled,
                  "astronomy_utc_millis",
                  e.astronomyUtcMillis,
                  "observer_latitude_deg",
                  e.observerLatitudeDegrees,
                  "observer_longitude_deg",
                  e.observerLongitudeDegrees);
       float sunX = std::numeric_limits<float>::quiet_NaN();
       float sunY = std::numeric_limits<float>::quiet_NaN();
       float sunZ = std::numeric_limits<float>::quiet_NaN();
       readFields(state,
                  "sun_direction_x",
                  sunX,
                  "sun_direction_y",
                  sunY,
                  "sun_direction_z",
                  sunZ,
                  "sun_azimuth_deg",
                  e.sunAzimuthDegrees,
                  "sun_altitude_deg",
                  e.sunAltitudeDegrees);
       if(std::isfinite(sunX) && std::isfinite(sunY) && std::isfinite(sunZ)) {
        e.sunDirectionX = sunX;
        e.sunDirectionY = sunY;
        e.sunDirectionZ = sunZ;
        e.solarDirectionValid = true;
       }
      }
      if(e.stage == WorldRenderStage::Stars && e.moment == RenderHookMoment::Before) {
       readField(state, "star_brightness", e.starBrightness);
      }
     });
#else
 (void)e;
#endif
}
void luaHookChunkGeneration(world::gen::ChunkGenerationEvent& e) {
 if(e.context.world != nullptr && e.context.world->isRemote()) {
  return;
 }
 LuaChunkContext::Scope scope(
     e.context.chunk, e.context.world, e.context.chunkX, e.context.chunkZ, chunkWriteModeForStage(e.stage));
 dispatchLuaHook(
     static_cast<int>(LuaEventId::ChunkGeneration),
     [&e](lua_State* state) {
      setFields(state,
                "stage",
                chunkStageName(e.stage),
                "moment",
                e.moment == world::gen::HookMoment::Before ? "before" : "after",
                "cancel_vanilla",
                e.cancelVanilla,
                "vanilla_stage_ran",
                e.vanillaStageRan,
                "world_seed",
                static_cast<std::int64_t>(e.context.worldSeed));
      setWorldContextFields(state, e.context.world);
      setFields(state, "mod_generation", e.context.modGeneration, "is_overworld", e.context.overworld);
      setChunkContextFields(state);
      pushChunkObject(state);
      luaApi().setfield(state, -2, "chunk");
     },
     [&e](lua_State* state) { readField(state, "cancel_vanilla", e.cancelVanilla); });
}
void luaHookScreenRegion(ScreenRegionEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 const bool renderPhase = e.phase == ScreenRegionPhase::Render;
 if(renderPhase) {
  luaGuiDrawPushScope();
 }
#endif
 dispatchLuaHook(
     static_cast<int>(LuaEventId::ScreenRegion),
     [&e](lua_State* state) {
      const char* phaseName = "render";
      if(e.phase == ScreenRegionPhase::MouseClick) {
       phaseName = "mouse_click";
      } else if(e.phase == ScreenRegionPhase::MouseScroll) {
       phaseName = "mouse_scroll";
      }
      setFields(state,
                "phase_name",
                phaseName,
                "screen_id",
                std::string(e.screenId),
                "region",
                std::string(e.region),
                "mouse_x",
                e.mouseX,
                "mouse_y",
                e.mouseY,
                "button",
                e.button,
                "scroll_delta",
                e.scrollDelta,
                "x",
                e.x,
                "y",
                e.y,
                "width",
                e.width,
                "height",
                e.height,
                "handled",
                e.handled);
     },
     [&e](lua_State* state) { readFields(state, "handled", e.handled, "width", e.width, "height", e.height); });
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(renderPhase) {
  luaGuiDrawPopScope();
 }
#endif
}
void luaHookWorldSpawnSearch(WorldSpawnSearchEvent& e) {
 if(e.world != nullptr && e.world->isRemote()) {
  return;
 }
 dispatchLuaHook(
     static_cast<int>(LuaEventId::WorldSpawnSearch),
     [&e](lua_State* state) {
      setFields(state, "x", e.x, "y", e.y, "z", e.z, "resolved", e.resolved);
      setWorldContextFields(state, e.world);
     },
     [&e](lua_State* state) { readFields(state, "x", e.x, "y", e.y, "z", e.z, "resolved", e.resolved); });
}
void luaHookScreenUi(ScreenUiEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(e.context == nullptr || e.context->screen == nullptr) {
  return;
 }
 g_activeScreenUi = e.context;
 int stackedY = e.context->stackedButtonY != nullptr ? *e.context->stackedButtonY : 0;
 const bool trackStacked = e.context->stackedButtonY != nullptr;
 dispatchLuaHook(
     static_cast<int>(LuaEventId::ScreenUi),
     [&e](lua_State* state) {
      setFields(state, "screen_id", std::string(e.context->screenId), "region", std::string(e.context->region));
      pushHostFieldsTable(state, e.context->screen);
      luaApi().setfield(state, -2, "host_fields");
      pushScreenUiTable(state);
      luaApi().setfield(state, -2, "ui");
     },
     [](lua_State*) {});
 if(trackStacked && e.context->stackedButtonY != nullptr) {
  *e.context->stackedButtonY = stackedY;
 }
 g_activeScreenUi = nullptr;
#else
 (void)e;
#endif
}
void luaHookScreenEvent(LuaScreenEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(g_activeLuaScreen == nullptr) {
  return;
 }
 static constexpr const char* kPhaseNames[] = {"init", "render", "tick", "key", "mouse", "scroll", "close"};
 dispatchLuaHook(
     static_cast<int>(LuaEventId::ScreenEvent),
     [&e](lua_State* state) {
      setFields(state,
                "screen_id",
                g_activeLuaScreen->id(),
                "phase",
                kPhaseNames[static_cast<int>(e.phase)],
                "width",
                g_activeLuaScreen->width(),
                "height",
                g_activeLuaScreen->height(),
                "mouse_x",
                e.mouseX,
                "mouse_y",
                e.mouseY,
                "x",
                e.mouseX,
                "y",
                e.mouseY,
                "tick_delta",
                e.tickDelta,
                "key",
                e.keyCode,
                "char",
                static_cast<int>(static_cast<unsigned char>(e.character)),
                "button",
                e.button,
                "released",
                e.released,
                "delta",
                e.scrollDelta,
                "handled",
                e.handled);
     },
     [&e](lua_State* state) { readField(state, "handled", e.handled); });
#else
 (void)e;
#endif
}
void luaHookPreEntityRender(PreEntityRenderEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 dispatchLuaHook(
     static_cast<int>(LuaEventId::PreEntityRender),
     [&e](lua_State* state) {
      setFields(state,
                "entity_id",
                e.entityId,
                "entity_type",
                e.entityType,
                "tick_delta",
                e.tickDelta,
                "canceled",
                e.canceled);
      pushItemEntityFields(state, e.entity);
     },
     [&e](lua_State* state) { readField(state, "canceled", e.canceled); });
#else
 (void)e;
#endif
}
void luaHookPreTileEntityRender(PreTileEntityRenderEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 dispatchLuaHook(
     static_cast<int>(LuaEventId::PreTileEntityRender),
     [&e](lua_State* state) {
      setFields(
          state, "x", e.x, "y", e.y, "z", e.z, "id", e.id, "tick_delta", e.tickDelta, "canceled", e.canceled);
     },
     [&e](lua_State* state) { readField(state, "canceled", e.canceled); });
#else
 (void)e;
#endif
}
void luaHookEntitySpawn(EntitySpawnEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 dispatchLuaHook(
     static_cast<int>(LuaEventId::EntitySpawn),
     [&e](lua_State* state) {
      setFields(state, "entity_id", e.entityId, "entity_type", e.entityType);
      pushItemEntityFields(state, e.entity);
     },
     [&e](lua_State* s) { kNoRead(s, e); });
#else
 (void)e;
#endif
}
void luaHookEntityRemove(EntityRemoveEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 dispatchLuaHook(
     static_cast<int>(LuaEventId::EntityRemove),
     [&e](lua_State* state) {
      setFields(state, "entity_id", e.entityId, "entity_type", e.entityType);
      pushItemEntityFields(state, e.entity);
     },
     [&e](lua_State* s) { kNoRead(s, e); });
#else
 (void)e;
#endif
}
} // namespace net::minecraft::mod::runtime
