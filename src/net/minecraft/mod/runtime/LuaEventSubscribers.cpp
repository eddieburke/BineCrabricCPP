#include "net/minecraft/mod/runtime/LuaEventSubscribers.hpp"

#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaBlockEntityBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#endif
#include <algorithm>
#include <string>
#include <unordered_map>

#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;

namespace {
using LuaEventSubscriber = void (*)(const std::shared_ptr<ModHost::LoadedLuaMod>&, int, int);
using LuaMod = std::shared_ptr<ModHost::LoadedLuaMod>;

// Standard shape shared by most events: subscribe, then push fields to the Lua
// event table and read mutations back. Events needing scopes/guards subscribe
// by hand below.
template <typename Event, typename Push, typename Read>
void subscribeLua(const LuaMod& mod, int ref, int priority, Push push, Read read) {
    hooks().subscribe<Event>(priority, [mod, ref, push, read](Event& e) {
        callLuaEvent(mod, ref, [&](lua_State* state) { push(state, e); }, [&](lua_State* state) { read(state, e); });
    });
}

constexpr auto kNoRead = [](lua_State*, auto&) {};

[[maybe_unused]] [[nodiscard]] ChunkWriteMode chunkWriteModeForStage(world::gen::ChunkStage stage) {
    switch (stage) {
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
    switch (stage) {
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

[[nodiscard]] const char* renderStageName(WorldRenderStage stage) {
    switch (stage) {
        case WorldRenderStage::Sky:
            return "sky";
        case WorldRenderStage::Stars:
            return "stars";
        case WorldRenderStage::Clouds:
            return "clouds";
    }
    return "unknown";
}

[[nodiscard]] const char* renderMomentName(RenderHookMoment moment) {
    return moment == RenderHookMoment::Before ? "before" : "after";
}

void setChunkContextFields(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
    setFields(state,
              "chunk_x",
              LuaChunkContext::activeChunkX(),
              "chunk_z",
              LuaChunkContext::activeChunkZ(),
              "has_chunk",
              LuaChunkContext::hasActiveChunk());
#else
    setFields(state, "chunk_x", 0, "chunk_z", 0, "has_chunk", false);
#endif
}

const std::unordered_map<std::string_view, LuaEventSubscriber>& luaEventSubscribers() {
    static const std::unordered_map<std::string_view, LuaEventSubscriber> kSubscribers = {
        {"client_tick",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<ClientTickEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, ClientTickEvent& e) { setClientTickFields(state, e); },
                 kNoRead);
         }},
        {"render_targets",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<RenderTargetsEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, RenderTargetsEvent& e) { setFields(state, "tick_delta", e.tickDelta); },
                 kNoRead);
         }},
        {"key_press",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<KeyPressEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, KeyPressEvent& e) {
                     setFields(state, "key", e.key, "pressed", e.pressed, "repeat", e.repeat, "handled", e.handled);
                 },
                 [](lua_State* state, KeyPressEvent& e) { readField(state, "handled", e.handled); });
         }},
        {"mouse_button",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<MouseButtonEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, MouseButtonEvent& e) {
                     setFields(state, "button", e.button, "pressed", e.pressed, "handled", e.handled);
                 },
                 [](lua_State* state, MouseButtonEvent& e) { readField(state, "handled", e.handled); });
         }},
        {"raycast",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<RaycastEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, RaycastEvent& e) {
                     const char* typeName = "none";
                     if (e.hasHit) {
                         typeName = e.type == HitResultType::ENTITY ? "entity" : "block";
                     }
                     setFields(state,
                               "has_hit",
                               e.hasHit,
                               "type",
                               typeName,
                               "hit_x",
                               e.hitX,
                               "hit_y",
                               e.hitY,
                               "hit_z",
                               e.hitZ,
                               "block_x",
                               e.blockX,
                               "block_y",
                               e.blockY,
                               "block_z",
                               e.blockZ,
                               "side",
                               e.side,
                               "block_id",
                               e.blockId,
                               "block_name",
                               blockWireNameFromId(e.blockId),
                               "item_id",
                               e.blockId);
                     if (e.entity != nullptr) {
                         const auto pos = e.entity->position();
                         setFields(state,
                                   "entity_id",
                                   entity::EntityRegistry::getId(*e.entity),
                                   "entity_raw_id",
                                   entity::EntityRegistry::getRawId(*e.entity),
                                   "entity_x",
                                   pos.x,
                                   "entity_y",
                                   pos.y,
                                   "entity_z",
                                   pos.z);
                     }
                 },
                 kNoRead);
         }},
        {"fov",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<FovEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, FovEvent& e) { setFields(state, "tick_delta", e.tickDelta, "fov", e.fov); },
                 [](lua_State* state, FovEvent& e) { readField(state, "fov", e.fov); });
         }},
        {"camera_setup",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<CameraSetupEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, CameraSetupEvent& e) {
                     setFields(state,
                               "tick_delta",
                               e.tickDelta,
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
                               "roll",
                               e.roll,
                               "custom_view",
                               e.customView,
                               "hide_first_person_hand",
                               e.hideFirstPersonHand);
                 },
                 [](lua_State* state, CameraSetupEvent& e) {
                     readFields(state,
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
                                "roll",
                                e.roll,
                                "custom_view",
                                e.customView,
                                "hide_first_person_hand",
                                e.hideFirstPersonHand);
                 });
         }},
        {"player_travel",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<PlayerTravelEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, PlayerTravelEvent& e) {
                     setFields(state,
                               "sideways",
                               e.sideways,
                               "forward",
                               e.forward,
                               "speed_multiplier",
                               e.speedMultiplier,
                               "has_player",
                               e.player != nullptr);
#ifdef MINECRAFT_NATIVE_EXPORTS
                     const client::Minecraft* client = client::Minecraft::INSTANCE;
                     const void* player = static_cast<const void*>(e.player);
                     const bool isLocal = player != nullptr && client != nullptr &&
                                          (static_cast<const void*>(client->player) == player ||
                                           static_cast<const void*>(client->camera) == player);
                     setField(state, "is_local_player", isLocal);
#else
                     setField(state, "is_local_player", false);
#endif
                 },
                 [](lua_State* state, PlayerTravelEvent& e) {
                     readFields(
                         state, "sideways", e.sideways, "forward", e.forward, "speed_multiplier", e.speedMultiplier);
                 });
         }},
        {"tick_rate",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<TickRateEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, TickRateEvent& e) {
                     setFields(state, "target_tps", e.targetTps, "tps_scale", e.tpsScale);
                 },
                 [](lua_State* state, TickRateEvent& e) {
                     readFields(state, "target_tps", e.targetTps, "tps_scale", e.tpsScale);
                 });
         }},
        {"world_start",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<WorldStartEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, WorldStartEvent& e) {
                     setFields(state,
                               "save_name",
                               e.saveName != nullptr ? *e.saveName : std::string(),
                               "new_world",
                               e.newWorld);
                 },
                 kNoRead);
         }},
        {"world_open",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<WorldOpenEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, WorldOpenEvent& e) {
                     setFields(state,
                               "save_name",
                               e.saveName != nullptr ? *e.saveName : std::string(),
                               "new_world",
                               e.newWorld);
                     if (e.options != nullptr) {
                         pushStringMap(state, *e.options);
                     } else {
                         pushStringMap(state, {});
                     }
                     luaApi().setfield(state, -2, "options");
                 },
                 kNoRead);
         }},
        {"world_tick",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<WorldTickEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, WorldTickEvent& e) {
                     setFields(state, "client_world", e.clientWorld, "before", e.before);
                 },
                 kNoRead);
         }},
        {"tile_entity_tick",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             hooks().subscribe<TileEntityTickEvent>(priority, [mod, ref](TileEntityTickEvent& e) {
                 if (e.entity == nullptr) {
                     return;
                 }
                 if (!e.animationTicked) {
                     tickTileEntityAnimation(e.entity);
                     e.animationTicked = true;
                 }
                 callLuaEvent(
                     mod,
                     ref,
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
                                   "client_world",
                                   e.clientWorld,
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
             });
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: tile_entity_tick");
#endif
         }},
        {"create_world",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<CreateWorldEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, CreateWorldEvent& e) {
                     setFields(state,
                               "save_name",
                               e.saveName != nullptr ? *e.saveName : std::string(),
                               "seed",
                               e.seed,
                               "canceled",
                               e.canceled);
                     pushStringMap(state, e.options);
                     luaApi().setfield(state, -2, "options");
                 },
                 [](lua_State* state, CreateWorldEvent& e) {
                     readField(state, "canceled", e.canceled);
                     readStringMapField(state, -1, "options", e.options);
                 });
         }},
        {"block_interact",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<BlockInteractEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, BlockInteractEvent& e) {
                     setFields(state,
                               "x",
                               e.x,
                               "y",
                               e.y,
                               "z",
                               e.z,
                               "block_id",
                               getBlockIdAt(e.world, e.x, e.y, e.z),
                               "side",
                               e.side,
                               "right_click",
                               e.rightClick,
                               "canceled",
                               e.canceled,
                               "handled",
                               e.handled,
                               "has_player",
                               e.player != nullptr,
                               "has_item",
                               e.stack != nullptr && !e.stack->empty());
                     if (e.stack != nullptr && !e.stack->empty()) {
                         setFields(state,
                                   "item_id",
                                   e.stack->itemId,
                                   "item_count",
                                   e.stack->count,
                                   "item_damage",
                                   e.stack->damage,
                                   "item_max_damage",
                                   e.stack->getMaxDamage(),
                                   "item_damageable",
                                   e.stack->isDamageable());
                     }
                 },
                 [](lua_State* state, BlockInteractEvent& e) {
                     readFields(state, "canceled", e.canceled, "handled", e.handled);
                     if (e.stack != nullptr && e.stack->isDamageable()) {
                         e.stack->damage = std::clamp(
                             luaIntField(state, -1, "item_damage", e.stack->damage), 0, e.stack->getMaxDamage());
                     }
                 });
         }},
        {"entity_interact",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<EntityInteractEvent>(priority, [mod, ref](EntityInteractEvent& e) {
                 if (e.player != nullptr && e.player->world != nullptr && e.player->world->isRemote()) {
                     return;
                 }
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         setFields(state,
                                   "attack",
                                   e.attack,
                                   "canceled",
                                   e.canceled,
                                   "handled",
                                   e.handled,
                                   "has_player",
                                   e.player != nullptr,
                                   "has_target",
                                   e.target != nullptr);
                     },
                     [&e](lua_State* state) { readFields(state, "canceled", e.canceled, "handled", e.handled); });
             });
         }},
        {"attack_damage",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<AttackDamageEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, AttackDamageEvent& e) {
                     setFields(state,
                               "damage",
                               e.damage,
                               "critical",
                               e.critical,
                               "canceled",
                               e.canceled,
                               "fall_distance",
                               e.fallDistance,
                               "on_ground",
                               e.onGround,
                               "target_x",
                               e.targetX,
                               "target_y",
                               e.targetY,
                               "target_z",
                               e.targetZ,
                               "has_player",
                               e.player != nullptr,
                               "has_target",
                               e.target != nullptr);
                 },
                 [](lua_State* state, AttackDamageEvent& e) {
                     readFields(state, "damage", e.damage, "critical", e.critical, "canceled", e.canceled);
                 });
         }},
        {"entity_teleport",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<EntityTeleportEvent>(priority, [mod, ref](EntityTeleportEvent& e) {
                 if (e.entity != nullptr && e.entity->world != nullptr && e.entity->world->isRemote()) {
                     return;
                 }
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         setFields(state,
                                   "id",
                                   e.entity != nullptr ? e.entity->id : -1,
                                   "type",
                                   e.entity != nullptr ? net::minecraft::entity::EntityRegistry::getId(*e.entity)
                                                       : std::string(),
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
                         readFields(state,
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
                                    e.canceled);
                     });
             });
         }},
        {"world_color",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<WorldColorEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, WorldColorEvent& e) {
                     setFields(state,
                               "partial_ticks",
                               e.partialTicks,
                               "r",
                               e.color.x,
                               "g",
                               e.color.y,
                               "b",
                               e.color.z,
                               "kind",
                               e.kind == WorldColorKind::Sky ? "sky" : "fog");
                     setWorldContextFields(state, e.world);
#ifdef MINECRAFT_NATIVE_EXPORTS
                     if (e.world != nullptr) {
                         setFields(state,
                                   "celestial",
                                   static_cast<double>(normalizedCelestial(e.world, e.partialTicks)),
                                   "world_time",
                                   static_cast<double>(e.world->getTime() % 24000ULL),
                                   "is_night",
                                   worldIsNight(e.world));
                     }
#endif
                 },
                 [](lua_State* state, WorldColorEvent& e) {
                     const auto component = [state](const char* name, double fallback) {
                         const float value = luaFloatField(state, -1, name, static_cast<float>(fallback));
                         return std::isfinite(value) ? static_cast<double>(std::clamp(value, 0.0f, 1.0f)) : fallback;
                     };
                     e.color.x = component("r", e.color.x);
                     e.color.y = component("g", e.color.y);
                     e.color.z = component("b", e.color.z);
                 });
         }},
        {"entity_render",
         [](const LuaMod& mod, int ref, int priority) {
             subscribeLua<EntityRenderEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, EntityRenderEvent& e) {
                     setFields(state,
                               "entity_id",
                               e.entityId,
                               "entity_type",
                               e.entityType,
                               "is_player",
                               e.isPlayer,
                               "tick_delta",
                               e.tickDelta);
                     luaApi().createtable(state, 0, 12);
                     setFields(state,
                               "body_yaw",
                               e.pose.bodyYaw,
                               "head_yaw",
                               e.pose.headYaw,
                               "head_pitch",
                               e.pose.headPitch,
                               "limb_swing",
                               e.pose.limbSwing,
                               "limb_distance",
                               e.pose.limbDistance,
                               "yaw",
                               e.pose.yaw,
                               "pitch",
                               e.pose.pitch,
                               "roll",
                               e.pose.roll,
                               "scale",
                               e.pose.scale,
                               "offset_x",
                               e.pose.offsetX,
                               "offset_y",
                               e.pose.offsetY,
                               "offset_z",
                               e.pose.offsetZ);
                     luaApi().createtable(state, 0, static_cast<int>(e.pose.parts.size()));
                     for (const auto& [name, part] : e.pose.parts) {
                         luaApi().createtable(state, 0, 3);
                         setFields(state, "yaw", part.yaw, "pitch", part.pitch, "roll", part.roll);
                         luaApi().setfield(state, -2, name.c_str());
                     }
                     luaApi().setfield(state, -2, "parts");
                     luaApi().setfield(state, -2, "pose");
                 },
                 [](lua_State* state, EntityRenderEvent& e) {
                     luaApi().getfield(state, -1, "pose");
                     if (luaApi().type(state, -1) == kLuaTTable) {
                         readFields(state,
                                    "body_yaw",
                                    e.pose.bodyYaw,
                                    "head_yaw",
                                    e.pose.headYaw,
                                    "head_pitch",
                                    e.pose.headPitch,
                                    "limb_swing",
                                    e.pose.limbSwing,
                                    "limb_distance",
                                    e.pose.limbDistance,
                                    "yaw",
                                    e.pose.yaw,
                                    "pitch",
                                    e.pose.pitch,
                                    "roll",
                                    e.pose.roll,
                                    "scale",
                                    e.pose.scale,
                                    "offset_x",
                                    e.pose.offsetX,
                                    "offset_y",
                                    e.pose.offsetY,
                                    "offset_z",
                                    e.pose.offsetZ);
                         luaApi().getfield(state, -1, "parts");
                         if (luaApi().type(state, -1) == kLuaTTable) {
                             luaApi().pushnil(state);
                             while (luaApi().next(state, -2) != 0) {
                                 if (luaApi().type(state, -2) == kLuaTString &&
                                     luaApi().type(state, -1) == kLuaTTable) {
                                     const std::string name = luaString(state, -2, "");
                                     if (!name.empty()) {
                                         ModelPartPose& part = e.pose.parts[name];
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
         }},
        {"world_render",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<WorldRenderEvent>(priority, [mod, ref](WorldRenderEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
                 ScopedModWorldDrawContext worldDrawScope{e.world, e.tickDelta};
                 const client::gl::preset::ModDraw modCaps;
#endif
                 callLuaEvent(
                     mod,
                     ref,
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
                         setWorldContextFields(state, e.world);
                         const client::render::FrameRenderCamera& frameCamera =
                             client::render::RenderCameraState::instance().frame();
                         const double cameraX = frameCamera.x;
                         const double cameraY = frameCamera.y;
                         const double cameraZ = frameCamera.z;
#ifdef MINECRAFT_NATIVE_EXPORTS
                         if (e.world != nullptr) {
                             setFields(state,
                                       "world_time",
                                       static_cast<double>(e.world->getTime() % 24000ULL),
                                       "celestial",
                                       static_cast<double>(normalizedCelestial(e.world, e.tickDelta)),
                                       "is_night",
                                       worldIsNight(e.world));
                         }
                         if (e.stage == WorldRenderStage::Clouds && e.world != nullptr &&
                             e.world->dimension != nullptr) {
                             float cloudBaseHeight =
                                 e.world->dimension->getCloudHeight() - static_cast<float>(cameraY) + 0.33f;
                             if (client::Minecraft* client = client::Minecraft::INSTANCE; client != nullptr) {
                                 cloudBaseHeight = client::option::cloudHeightOffset(
                                     cloudBaseHeight, client::option::resolve(client->options));
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
                         if (e.stage == WorldRenderStage::Sky && e.moment == RenderHookMoment::Before) {
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
                         }
                     });
             });
         }},
        {"chunk_generation",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<world::gen::ChunkGenerationEvent>(
                 priority, [mod, ref](world::gen::ChunkGenerationEvent& e) {
                     if (e.context.world != nullptr && e.context.world->isRemote()) {
                         return;
                     }
#ifdef MINECRAFT_NATIVE_EXPORTS
                     LuaChunkContext::Scope scope(e.context.chunk,
                                                  e.context.world,
                                                  e.context.chunkX,
                                                  e.context.chunkZ,
                                                  chunkWriteModeForStage(e.stage));
#endif
                     callLuaEvent(
                         mod,
                         ref,
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
                             setFields(
                                 state, "mod_generation", e.context.modGeneration, "is_overworld", e.context.overworld);
                             setChunkContextFields(state);
                         },
                         [&e](lua_State* state) { readField(state, "cancel_vanilla", e.cancelVanilla); });
                 });
         }},
        {"screen_region",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<ScreenRegionEvent>(priority, [mod, ref](ScreenRegionEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
                 const bool renderPhase = e.phase == ScreenRegionPhase::Render;
                 const ScopedLuaGuiDraw drawScope(renderPhase);
#endif
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         const char* phaseName = "render";
                         if (e.phase == ScreenRegionPhase::MouseClick) {
                             phaseName = "mouse_click";
                         } else if (e.phase == ScreenRegionPhase::MouseScroll) {
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
                     [&e](lua_State* state) {
                         readFields(state, "handled", e.handled, "width", e.width, "height", e.height);
                     });
             });
         }},
        {"world_spawn_search",
         [](const LuaMod& mod, int ref, int priority) {
             hooks().subscribe<WorldSpawnSearchEvent>(priority, [mod, ref](WorldSpawnSearchEvent& e) {
                 if (e.world != nullptr && e.world->isRemote()) {
                     return;
                 }
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         setFields(state, "x", e.x, "y", e.y, "z", e.z, "resolved", e.resolved);
                         setWorldContextFields(state, e.world);
                     },
                     [&e](lua_State* state) {
                         readFields(state, "x", e.x, "y", e.y, "z", e.z, "resolved", e.resolved);
                     });
             });
         }},
        {"screen_ui",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             hooks().subscribe<ScreenUiEvent>(priority, [mod, ref](ScreenUiEvent& e) {
                 if (e.context == nullptr || e.context->screen == nullptr) {
                     return;
                 }
                 ActiveScreenUi* session = activeScreenUi();
                 session->context = e.context;
                 session->mod = mod.get();
                 session->stackedY = e.context->stackedButtonY != nullptr ? *e.context->stackedButtonY : 0;
                 session->trackStacked = e.context->stackedButtonY != nullptr;
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         setFields(state,
                                   "screen_id",
                                   std::string(e.context->screenId),
                                   "region",
                                   std::string(e.context->region));
                         pushHostFieldsTable(state, e.context->screen);
                         luaApi().setfield(state, -2, "host_fields");
                         pushScreenUiTable(state);
                         luaApi().setfield(state, -2, "ui");
                     },
                     [](lua_State*) {});
                 if (session->trackStacked && e.context->stackedButtonY != nullptr) {
                     *e.context->stackedButtonY = session->stackedY;
                 }
                 session->context = nullptr;
                 session->mod = nullptr;
                 session->trackStacked = false;
             });
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: screen_ui");
#endif
         }},
        {"screen_event",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             hooks().subscribe<LuaScreenEvent>(priority, [mod, ref](LuaScreenEvent& e) {
                 if (e.screen == nullptr) {
                     return;
                 }
                 static constexpr const char* kPhaseNames[] = {
                     "init",
                     "render",
                     "tick",
                     "key",
                     "mouse",
                     "scroll",
                     "close",
                 };
                 activeLuaScreen()->mod = mod.get();
                 callLuaEvent(
                     mod,
                     ref,
                     [&e](lua_State* state) {
                         setFields(state,
                                   "screen_id",
                                   e.screen->id(),
                                   "phase",
                                   kPhaseNames[static_cast<int>(e.phase)],
                                   "width",
                                   e.screen->width(),
                                   "height",
                                   e.screen->height(),
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
                 activeLuaScreen()->mod = nullptr;
             });
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: screen_event");
#endif
         }},
        {"pre_entity_render",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             subscribeLua<PreEntityRenderEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, PreEntityRenderEvent& e) {
                     setFields(state,
                               "entity_id",
                               e.entityId,
                               "entity_type",
                               e.entityType,
                               "tick_delta",
                               e.tickDelta,
                               "canceled",
                               e.canceled);
                     if (e.entityType == "Item" && e.entity != nullptr) {
                         const auto* item = dynamic_cast<const net::minecraft::entity::ItemEntity*>(e.entity);
                         if (item != nullptr) {
                             const ItemStack& stack = item->stack;
                             setFields(state,
                                       "item_id",
                                       stack.itemId,
                                       "item_count",
                                       stack.count,
                                       "item_damage",
                                       stack.damage);
                             const bool modTex =
                                 net::minecraft::client::render::item::ItemModelRenderer::usesModTexture(stack);
                             if (modTex) {
                                 const auto* spec = itemRegistrationSpecForId(stack.itemId);
                                 const std::string path = spec != nullptr ? spec->texturePath : std::string();
                                 setFields(state, "texture_path", path, "mod_texture", true, "atlas_index", -1);
                             } else {
                                 setFields(
                                     state,
                                     "texture_path",
                                     net::minecraft::client::render::item::ItemModelRenderer::spriteAtlasPath(stack),
                                     "mod_texture",
                                     false,
                                     "atlas_index",
                                     stack.getTextureId());
                             }
                         }
                     }
                 },
                 [](lua_State* state, PreEntityRenderEvent& e) { readField(state, "canceled", e.canceled); });
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: pre_entity_render");
#endif
         }},
        {"pre_tile_entity_render",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             subscribeLua<PreTileEntityRenderEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, PreTileEntityRenderEvent& e) {
                     setFields(state,
                               "x",
                               e.x,
                               "y",
                               e.y,
                               "z",
                               e.z,
                               "id",
                               e.id,
                               "tick_delta",
                               e.tickDelta,
                               "canceled",
                               e.canceled);
                 },
                 [](lua_State* state, PreTileEntityRenderEvent& e) { readField(state, "canceled", e.canceled); });
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: pre_tile_entity_render");
#endif
         }},
        {"entity_spawn",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             subscribeLua<EntitySpawnEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, EntitySpawnEvent& e) {
                     setFields(state, "entity_id", e.entityId, "entity_type", e.entityType);
                     if (e.entityType == "Item" && e.entity != nullptr) {
                         const auto* item = dynamic_cast<const net::minecraft::entity::ItemEntity*>(e.entity);
                         if (item != nullptr) {
                             const ItemStack& stack = item->stack;
                             setFields(state,
                                       "item_id",
                                       stack.itemId,
                                       "item_count",
                                       stack.count,
                                       "item_damage",
                                       stack.damage);
                             const bool modTex =
                                 net::minecraft::client::render::item::ItemModelRenderer::usesModTexture(stack);
                             if (modTex) {
                                 const auto* spec = itemRegistrationSpecForId(stack.itemId);
                                 const std::string path = spec != nullptr ? spec->texturePath : std::string();
                                 setFields(state, "texture_path", path, "mod_texture", true, "atlas_index", -1);
                             } else {
                                 setFields(
                                     state,
                                     "texture_path",
                                     net::minecraft::client::render::item::ItemModelRenderer::spriteAtlasPath(stack),
                                     "mod_texture",
                                     false,
                                     "atlas_index",
                                     stack.getTextureId());
                             }
                         }
                     }
                 },
                 kNoRead);
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: entity_spawn");
#endif
         }},
        {"entity_remove",
         [](const LuaMod& mod, int ref, int priority) {
#ifdef MINECRAFT_NATIVE_EXPORTS
             subscribeLua<EntityRemoveEvent>(
                 mod,
                 ref,
                 priority,
                 [](lua_State* state, EntityRemoveEvent& e) {
                     setFields(state, "entity_id", e.entityId, "entity_type", e.entityType);
                     if (e.entityType == "Item" && e.entity != nullptr) {
                         const auto* item = dynamic_cast<const net::minecraft::entity::ItemEntity*>(e.entity);
                         if (item != nullptr) {
                             const ItemStack& stack = item->stack;
                             setFields(state,
                                       "item_id",
                                       stack.itemId,
                                       "item_count",
                                       stack.count,
                                       "item_damage",
                                       stack.damage);
                             const bool modTex =
                                 net::minecraft::client::render::item::ItemModelRenderer::usesModTexture(stack);
                             if (modTex) {
                                 const auto* spec = itemRegistrationSpecForId(stack.itemId);
                                 const std::string path = spec != nullptr ? spec->texturePath : std::string();
                                 setFields(state, "texture_path", path, "mod_texture", true, "atlas_index", -1);
                             } else {
                                 setFields(
                                     state,
                                     "texture_path",
                                     net::minecraft::client::render::item::ItemModelRenderer::spriteAtlasPath(stack),
                                     "mod_texture",
                                     false,
                                     "atlas_index",
                                     stack.getTextureId());
                             }
                         }
                     }
                 },
                 kNoRead);
#else
             (void) ref;
             (void) priority;
             runtimeLog(mod->modId, "warn", "unsupported Lua hook event: entity_remove");
#endif
         }},
    };
    return kSubscribers;
}
}  // namespace

bool isSupportedLuaEvent(std::string_view event) {
    return luaEventSubscribers().contains(event);
}

void subscribeLuaCallback(const std::shared_ptr<ModHost::LoadedLuaMod>& mod,
                          const ModHost::LoadedLuaMod::Callback& callback) {
    const auto& subscribers = luaEventSubscribers();
    const auto it = subscribers.find(toLowerCopy(callback.event));
    if (it == subscribers.end()) {
        runtimeLog(mod->modId, "warn", "unsupported Lua hook event: " + callback.event);
        return;
    }
    it->second(mod, callback.functionRef, callback.priority);
}
}  // namespace net::minecraft::mod::runtime
