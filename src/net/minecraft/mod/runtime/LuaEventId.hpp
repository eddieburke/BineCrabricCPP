#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace net::minecraft::mod::runtime {
enum class LuaEventId : std::uint8_t {
 ClientTick = 0,
 RenderFrame,
 FirstPersonHand,
 KeyPress,
 MouseButton,
 Raycast,
 Fov,
 CameraSetup,
 PlayerTravel,
 TickRate,
 WorldStart,
 WorldOpen,
 WorldTick,
 EntityTick,
 TileEntityTick,
 CreateWorld,
 BlockInteract,
 EntityInteract,
 AttackDamage,
 EntityTeleport,
 WorldColor,
 FogSettings,
 EntityRender,
 WorldRender,
 ChunkGeneration,
 ScreenRegion,
 WorldSpawnSearch,
 ScreenUi,
 ScreenEvent,
 PreEntityRender,
 PreTileEntityRender,
 EntitySpawn,
 EntityRemove,
 Count
};
inline constexpr std::size_t kLuaEventCount = static_cast<std::size_t>(LuaEventId::Count);
inline constexpr std::string_view kLuaEventNames[kLuaEventCount] = {
    "client_tick",
    "render_frame",
    "first_person_hand",
    "key_press",
    "mouse_button",
    "raycast",
    "fov",
    "camera_setup",
    "player_travel",
    "tick_rate",
    "world_start",
    "world_open",
    "world_tick",
    "entity_tick",
    "tile_entity_tick",
    "create_world",
    "block_interact",
    "entity_interact",
    "attack_damage",
    "entity_teleport",
    "world_color",
    "fog_settings",
    "entity_render",
    "world_render",
    "chunk_generation",
    "screen_region",
    "world_spawn_search",
    "screen_ui",
    "screen_event",
    "pre_entity_render",
    "pre_tile_entity_render",
    "entity_spawn",
    "entity_remove",
};
[[nodiscard]] constexpr int luaEventIndexOf(std::string_view name) {
 for(std::size_t i = 0; i < kLuaEventCount; ++i) {
  if(kLuaEventNames[i] == name) {
   return static_cast<int>(i);
  }
 }
 return -1;
}
[[nodiscard]] constexpr int luaEventIndexOf(LuaEventId id) {
 return static_cast<int>(id);
}
} // namespace net::minecraft::mod::runtime
