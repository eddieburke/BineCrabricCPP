#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include <optional>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEntityBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
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
[[nodiscard]] ChunkWriteMode chunkWriteModeForStage(world::gen::ChunkStage stage) {
 switch(stage) {
 case world::gen::ChunkStage::Terrain:
 case world::gen::ChunkStage::Surface:
 case world::gen::ChunkStage::Carver:
  return ChunkWriteMode::RawGeneration;
 case world::gen::ChunkStage::Features:
  return ChunkWriteMode::ChunkApi;
 default:
  break;
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
 default:
  break;
 }
 return "unknown";
}
#ifdef MINECRAFT_NATIVE_EXPORTS
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
 pushItemStackFields(state, item->stack);
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
 ModContextScope scope(world, eventPlayer(e));
 dispatchLuaHook(eventIndex, fill, apply);
}
} // namespace
namespace {
struct LuaHookEntry {
 std::shared_ptr<ModHost::LoadedLuaMod> mod;
 int functionRef = 0;
 std::uint16_t worldRenderStageMask = 0xFFFFu;
 std::uint8_t worldRenderMomentMask = 0xFFu;
 std::string entityTypeFilter;
};
using LuaHookEntries = std::vector<LuaHookEntry>;
std::array<std::shared_ptr<const LuaHookEntries>, kLuaEventCount> gHookTable{};
std::array<bool, kLuaEventCount> gHasLuaHookArray{};
bool gHookTableValid = false;
void rebuildHookTable() {
 struct PendingEntry {
  LuaHookEntry entry;
  int priority = 0;
  std::size_t order = 0;
 };
 std::array<std::vector<PendingEntry>, kLuaEventCount> pending{};
 std::size_t order = 0;
 for(const auto& mod : loadedLuaMods()) {
  if(mod == nullptr || !mod->active) {
   continue;
  }
  for(const auto& cb : mod->callbacks) {
   if(cb.eventIndex < 0 || static_cast<std::size_t>(cb.eventIndex) >= kLuaEventCount) {
    continue;
   }
   pending[static_cast<std::size_t>(cb.eventIndex)].push_back(
       {{mod, cb.functionRef, cb.worldRenderStageMask, cb.worldRenderMomentMask, cb.entityTypeFilter},
        cb.priority,
        order++});
  }
 }
 for(std::size_t i = 0; i < kLuaEventCount; ++i) {
  std::stable_sort(pending[i].begin(), pending[i].end(), [](const PendingEntry& a, const PendingEntry& b) {
   return a.priority != b.priority ? a.priority > b.priority : a.order < b.order;
  });
  LuaHookEntries entries;
  entries.reserve(pending[i].size());
  for(auto& entry : pending[i]) {
   entries.push_back(std::move(entry.entry));
  }
  gHasLuaHookArray[i] = !entries.empty();
  gHookTable[i] = std::make_shared<const LuaHookEntries>(std::move(entries));
 }
 gHookTableValid = true;
}
[[nodiscard]] std::shared_ptr<const LuaHookEntries> luaHookEntries(std::size_t eventIndex) {
 if(!gHookTableValid) {
  rebuildHookTable();
 }
 return gHookTable[eventIndex];
}
} // namespace
void invalidateLuaHookCache() {
 gHookTableValid = false;
 gHasLuaHookArray.fill(false);
 for(auto& entries : gHookTable) {
  entries.reset();
 }
}
bool hasLuaHook(int eventIndex) {
 if(eventIndex < 0 || static_cast<std::size_t>(eventIndex) >= kLuaEventCount) {
  return false;
 }
 if(!gHookTableValid) {
  rebuildHookTable();
 }
 return gHasLuaHookArray[static_cast<std::size_t>(eventIndex)];
}
template <typename Fill, typename Read>
void dispatchLuaHook(int eventIndex,
                     Fill fill,
                     Read read,
                     std::uint16_t worldRenderStageBit,
                     std::uint8_t worldRenderMomentBit,
                     std::string_view entityType) {
 if(!isLuaModExecutionEnabled() || eventIndex < 0 || static_cast<std::size_t>(eventIndex) >= kLuaEventCount) {
  return;
 }
 const std::shared_ptr<const LuaHookEntries> entries = luaHookEntries(static_cast<std::size_t>(eventIndex));
 if(entries == nullptr) {
  return;
 }
 for(const LuaHookEntry& entry : *entries) {
  if(entry.mod == nullptr || !entry.mod->active) {
   continue;
  }
  if(worldRenderStageBit != 0 && (entry.worldRenderStageMask & worldRenderStageBit) == 0) {
   continue;
  }
  if(worldRenderMomentBit != 0 && (entry.worldRenderMomentMask & worldRenderMomentBit) == 0) {
   continue;
  }
  if(!entityType.empty() && !entry.entityTypeFilter.empty() && entry.entityTypeFilter != entityType) {
   continue;
  }
  callLuaEvent(entry.mod, entry.functionRef, fill, read);
 }
}
[[nodiscard]] bool isSupportedLuaEvent(std::string_view event) {
 return luaEventIndexOf(event) >= 0;
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
constexpr std::size_t kChunkStageCount = static_cast<std::size_t>(world::gen::ChunkStage::Count);
std::array<std::vector<ChunkStageListenerEntry>, kChunkStageCount>& chunkStageListeners() {
 static std::array<std::vector<ChunkStageListenerEntry>, kChunkStageCount> value{};
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
 auto& list = chunkStageListeners()[chunkStageIndex(stage)];
 list.push_back({priority, std::move(listener)});
 std::stable_sort(list.begin(), list.end(), [](const ChunkStageListenerEntry& a, const ChunkStageListenerEntry& b) {
  return a.priority > b.priority;
 });
}
void fireChunkGeneration(world::gen::ChunkGenerationEvent& event) {
 for(const auto& entry : chunkStageListeners()[chunkStageIndex(event.stage)]) {
  entry.listener(event);
 }
 if(hasLuaHook(LuaEventId::ChunkGeneration)) {
  luaHookChunkGeneration(event);
 }
}
[[nodiscard]] std::size_t chunkStageListenerCount(world::gen::ChunkStage stage) {
 return chunkStageListeners()[chunkStageIndex(stage)].size();
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
      if(ev.frame != nullptr) {
       setFields(state,
                 "tick_delta",
                 ev.tickDelta,
                 "x",
                 ev.frame->x,
                 "y",
                 ev.frame->y,
                 "z",
                 ev.frame->z,
                 "yaw",
                 ev.frame->yaw,
                 "pitch",
                 ev.frame->pitch,
                 "roll",
                 ev.frame->roll,
                 "custom_view",
                 ev.frame->customView,
                 "hide_first_person_hand",
                 ev.frame->hideFirstPersonHand);
      }
     },
     [](lua_State* state, CameraSetupEvent& ev) {
      if(ev.frame != nullptr) {
       readFields(state,
                  "x",
                  ev.frame->x,
                  "y",
                  ev.frame->y,
                  "z",
                  ev.frame->z,
                  "yaw",
                  ev.frame->yaw,
                  "pitch",
                  ev.frame->pitch,
                  "roll",
                  ev.frame->roll,
                  "custom_view",
                  ev.frame->customView,
                  "hide_first_person_hand",
                  ev.frame->hideFirstPersonHand);
      }
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
void luaHookSnowIcePlacement(SnowIcePlacementEvent& e) {
 runLuaHook(
     LuaEventId::SnowIcePlacement,
     e,
     [](lua_State* state, SnowIcePlacementEvent& ev) {
      setFields(
          state, "x", ev.x, "y", ev.y, "z", ev.z, "place_snow", ev.placeSnow, "place_ice", ev.placeIce);
     },
     [](lua_State* state, SnowIcePlacementEvent& ev) {
      readFields(state, "place_snow", ev.placeSnow, "place_ice", ev.placeIce);
     });
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
void luaHookCelestialState(CelestialStateEvent& e) {
 runLuaHook(
     LuaEventId::CelestialState,
     e,
     [](lua_State* state, CelestialStateEvent& ev) {
      setFields(state,
                "tick_delta",
                ev.tickDelta,
                "celestial_angle",
                ev.celestialAngle,
                "sun_angle",
                ev.sunAngle,
                "shadow_angle",
                ev.shadowAngle,
                "moon_phase",
                ev.moonPhase,
                "sun_x",
                ev.sunX,
                "sun_y",
                ev.sunY,
                "sun_z",
                ev.sunZ,
                "moon_x",
                ev.moonX,
                "moon_y",
                ev.moonY,
                "moon_z",
                ev.moonZ,
                "is_day",
                ev.day,
                "override_directions",
                ev.overrideDirections);
      setWorldContextFields(state, ev.world);
     },
     [](lua_State* state, CelestialStateEvent& ev) {
      readFields(state,
                 "celestial_angle",
                 ev.celestialAngle,
                 "sun_angle",
                 ev.sunAngle,
                 "shadow_angle",
                 ev.shadowAngle,
                 "moon_phase",
                 ev.moonPhase,
                 "sun_x",
                 ev.sunX,
                 "sun_y",
                 ev.sunY,
                 "sun_z",
                 ev.sunZ,
                 "moon_x",
                 ev.moonX,
                 "moon_y",
                 ev.moonY,
                 "moon_z",
                 ev.moonZ,
                 "is_day",
                 ev.day,
                 "override_directions",
                 ev.overrideDirections);
     });
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
       // The frame's single celestial answer, published by GameRenderer::updateSunLight
       // before any render-path hook runs. Re-deriving the angle or night flag from the
       // raw clock here is what let hooks disagree with the sky the engine drew; the
       // day/night override (World::clientTimeMode) is folded in upstream, so hooks see
       // exactly what is rendered and no time_mode intermediate is needed.
       // see src/net/minecraft/client/render/celestial/CelestialState.hpp
       const net::minecraft::client::render::CelestialState& celestial =
           net::minecraft::client::render::core::celestialState();
       setFields(state,
                 "celestial_angle",
                 static_cast<double>(celestial.celestialAngle),
                 "sun_angle",
                 static_cast<double>(celestial.sunAngle),
                 "shadow_angle",
                 static_cast<double>(celestial.shadowAngle),
                 "is_day",
                 celestial.day,
                 "world_time",
                 static_cast<double>(ev.world->getTime() % 24000ULL));
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
                ev.density);
      setWorldContextFields(state, ev.world);
     },
     [](lua_State* state, FogSettingsEvent& ev) {
      readFields(state,
                 "enabled",
                 ev.enabled,
                 "spherical",
                 ev.spherical,
                 "exponential",
                 ev.exponential);
      ev.start = std::clamp(luaFloatField(state, -1, "start", ev.start), 0.0f, 1.0f);
      ev.end = std::clamp(luaFloatField(state, -1, "end", ev.end), 0.0f, 1.0f);
      ev.density = std::clamp(luaFloatField(state, -1, "density", ev.density), 0.0f, 1.0f);
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
 const std::uint16_t stageBit = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(e.stage));
 const std::uint8_t momentBit = static_cast<std::uint8_t>(1u << static_cast<unsigned int>(e.moment));
 const std::shared_ptr<const LuaHookEntries> entries =
     luaHookEntries(static_cast<std::size_t>(LuaEventId::WorldRender));
 if(entries == nullptr ||
    std::none_of(entries->begin(), entries->end(), [stageBit, momentBit](const LuaHookEntry& entry) {
     return entry.mod != nullptr && entry.mod->active && (entry.worldRenderStageMask & stageBit) != 0 &&
            (entry.worldRenderMomentMask & momentBit) != 0;
    })) {
  return;
 }
 const ModDrawLayer drawLayer = e.stage == WorldRenderStage::Clouds
                                    ? ModDrawLayer::Clouds
                                    : e.stage == WorldRenderStage::Sky || e.stage == WorldRenderStage::Stars
                                          ? ModDrawLayer::Sky
                                          : ModDrawLayer::Auto;
 ScopedModWorldDrawContext worldDrawScope{e.world, e.tickDelta, drawLayer};
 client::render::core::RenderStage renderStage = client::render::core::RenderStage::None;
 switch(e.stage) {
 case WorldRenderStage::Sky:
  renderStage = client::render::core::RenderStage::Sky;
  break;
 case WorldRenderStage::Stars:
  renderStage = client::render::core::RenderStage::Stars;
  break;
 case WorldRenderStage::OpaqueTerrain:
  renderStage = client::render::core::RenderStage::TerrainSolid;
  break;
 case WorldRenderStage::Entities:
  renderStage = client::render::core::RenderStage::Entities;
  break;
 case WorldRenderStage::LitParticles:
 case WorldRenderStage::Particles:
  renderStage = client::render::core::RenderStage::Particles;
  break;
 case WorldRenderStage::TranslucentTerrain:
  renderStage = client::render::core::RenderStage::TerrainTranslucent;
  break;
 case WorldRenderStage::Weather:
  renderStage = client::render::core::RenderStage::RainSnow;
  break;
 case WorldRenderStage::Clouds:
  renderStage = client::render::core::RenderStage::Clouds;
  break;
 case WorldRenderStage::Hand:
  renderStage = client::render::core::RenderStage::HandSolid;
  break;
 case WorldRenderStage::Framebuffer:
  break;
 }
 const client::render::core::RenderStageScope renderStageScope(renderStage);
 std::optional<ModContextScope> contextScope;
 if(e.world != nullptr) {
  contextScope.emplace(e.world, nullptr);
 }
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
                "star_brightness",
                e.starBrightness,
                "rain_strength",
                e.rainStrength,
                "astronomy_enabled",
                e.astronomyEnabled,
                "astronomy_utc_millis",
                e.astronomyUtcMillis,
                "observer_latitude_deg",
                e.observerLatitudeDeg,
                "observer_longitude_deg",
                e.observerLongitudeDeg);
      setWorldContextFields(state, e.world);
      const client::render::FrameRenderCamera& frameCamera =
          client::render::core::cameraFrame();
      const double cameraX = frameCamera.x;
      const double cameraY = frameCamera.y;
      const double cameraZ = frameCamera.z;
#ifdef MINECRAFT_NATIVE_EXPORTS
      if(e.world != nullptr) {
       // is_day comes from the frame's published celestial state, same as world_color
       // — the raw-clock re-derivation is what let mods see a different phase than
       // the sky/shadow map rendered. see src/net/minecraft/client/render/celestial/CelestialState.hpp
       setFields(state,
                 "world_time",
                 static_cast<double>(e.world->getTime() % 24000ULL),
                 "is_day",
                 net::minecraft::client::render::core::celestialState().day,
                 // World-space height of the dimension's cloud layer. Cloud mods
                 // (layered_clouds) draw their sheets at `cloud_base_height +
                 // (layer.height - 128)`; without this field they fell back to
                 // `128 - camera_y + 0.33` — eye-relative — and layered the clouds
                 // around the player, whitewashing everything beyond a chunk.
                 // see mods/layered_clouds/scripts/main.lua
                 "cloud_base_height",
                 e.world->dimension != nullptr ? static_cast<double>(e.world->dimension->getCloudHeight()) : 0.0);
      }
      const client::render::CelestialState& celestial = client::render::core::celestialState();
      setFields(state,
                "sun_x",
                celestial.sunDirectionWorld[0],
                "sun_y",
                celestial.sunDirectionWorld[1],
                "sun_z",
                celestial.sunDirectionWorld[2],
                "moon_x",
                celestial.moonDirectionWorld[0],
                "moon_y",
                celestial.moonDirectionWorld[1],
                "moon_z",
                celestial.moonDirectionWorld[2]);
      if(client::Minecraft::INSTANCE != nullptr) {
       const std::string& selectedPack = client::Minecraft::INSTANCE->options.shaderPack;
       const bool shaderPackActive = !selectedPack.empty() && selectedPack != "vanilla" && selectedPack != "off";
       setFields(state,
                 "shader_pack_active",
                 shaderPackActive,
                 "shader_pack_name",
                 shaderPackActive ? selectedPack : std::string("vanilla"));
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
      readFields(state,
                 "cancel_vanilla",
                 e.cancelVanilla,
                 "star_brightness",
                 e.starBrightness,
                 "rain_strength",
                 e.rainStrength,
                 "astronomy_enabled",
                 e.astronomyEnabled,
                 "astronomy_utc_millis",
                 e.astronomyUtcMillis,
                 "observer_latitude_deg",
                 e.observerLatitudeDeg,
                 "observer_longitude_deg",
                 e.observerLongitudeDeg);
      e.starBrightness = std::isfinite(e.starBrightness) ? std::clamp(e.starBrightness, 0.0f, 1.0f) : 0.0f;
      e.rainStrength = std::isfinite(e.rainStrength) ? std::clamp(e.rainStrength, 0.0f, 1.0f) : 0.0f;
     },
     stageBit,
     momentBit);
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
 ScreenUiContext* previousScreenUi = g_activeScreenUi;
 g_activeScreenUi = e.context;
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
 g_activeScreenUi = previousScreenUi;
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
 const std::string screenId = g_activeLuaScreen->id();
 const int screenWidth = g_activeLuaScreen->width();
 const int screenHeight = g_activeLuaScreen->height();
 dispatchLuaHook(
     static_cast<int>(LuaEventId::ScreenEvent),
     [&e, &screenId, screenWidth, screenHeight](lua_State* state) {
      setFields(state,
                "screen_id",
                screenId,
                "phase",
                kPhaseNames[static_cast<int>(e.phase)],
                "width",
                screenWidth,
                "height",
                screenHeight,
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
     [&e](lua_State* state) { readField(state, "canceled", e.canceled); },
     0,
     0,
     e.entityType);
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
 clearLocalPoseHook(e.entityId);
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
