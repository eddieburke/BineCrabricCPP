#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
namespace net::minecraft::client::render {
enum class WorldProgramId : std::uint8_t {
 TerrainSolid,
 TerrainCutout,
 TerrainTranslucent,
 Gui,
 GuiTextured,
 Item,
 Text,
 SkyBasic,
 SkyTextured,
 Basic,
 Textured,
 Line,
 Entities,
 EntitiesTranslucent,
 Lightning,
 Block,
 BlockTranslucent,
 Particles,
 ParticlesTranslucent,
 Clouds,
 Weather,
 Hand,
 DamagedBlock,
 Count
};
inline constexpr std::array<std::string_view, static_cast<std::size_t>(WorldProgramId::Count)> kWorldProgramKeys = {
    "gbuffers_terrain_solid", "gbuffers_terrain_cutout", "gbuffers_water", "gbuffers_gui",
    "gbuffers_gui_textured", "gbuffers_item", "gbuffers_text", "gbuffers_skybasic",
    "gbuffers_skytextured", "gbuffers_basic", "gbuffers_textured", "gbuffers_line", "gbuffers_entities",
    "gbuffers_entities_translucent", "gbuffers_lightning", "gbuffers_block",
    "gbuffers_block_translucent", "gbuffers_particles", "gbuffers_particles_translucent",
    "gbuffers_clouds", "gbuffers_weather", "gbuffers_hand", "gbuffers_damagedblock"};
[[nodiscard]] inline constexpr std::string_view worldProgramKey(WorldProgramId id) noexcept {
 return kWorldProgramKeys[static_cast<std::size_t>(id)];
}
[[nodiscard]] inline constexpr std::optional<WorldProgramId> worldProgramId(std::string_view key) noexcept {
 for(std::size_t index = 0; index < kWorldProgramKeys.size(); ++index) {
  if(kWorldProgramKeys[index] == key) return static_cast<WorldProgramId>(index);
 }
 return std::nullopt;
}
// Iris hangs the fog mode on the ShaderKey -- the kind of draw -- not on the program the
// key resolves to, so SKY_BASIC keeps FogMode.OFF even when gbuffers_skybasic falls back
// through gbuffers_basic. Every key off here is FogMode.OFF in 26.1: SKY_BASIC(_COLOR),
// SKY_TEXTURED(_COLOR), TEXTURED(_COLOR) and CRUMBLING. Gui/GuiTextured have no Iris key
// at all -- Iris never runs the interface through gbuffers -- and stay off with them.
// The shadow pass is off for every id (all SHADOW_* keys are FogMode.OFF); that is the
// caller's business because the same id resolves to a shadow program there.
// see third_party/mcp/iris/pipeline/programs/ShaderKey.java
[[nodiscard]] inline constexpr bool fogEnabled(WorldProgramId id) noexcept {
 return id != WorldProgramId::Gui && id != WorldProgramId::GuiTextured &&
        id != WorldProgramId::SkyBasic && id != WorldProgramId::SkyTextured &&
        id != WorldProgramId::Textured && id != WorldProgramId::DamagedBlock;
}
[[nodiscard]] inline constexpr bool bindsTextureAtlases(WorldProgramId id) noexcept {
 return id == WorldProgramId::TerrainSolid || id == WorldProgramId::TerrainCutout ||
        id == WorldProgramId::TerrainTranslucent || id == WorldProgramId::Item ||
        id == WorldProgramId::Entities || id == WorldProgramId::EntitiesTranslucent ||
        id == WorldProgramId::Lightning || id == WorldProgramId::Block ||
        id == WorldProgramId::BlockTranslucent || id == WorldProgramId::Particles ||
        id == WorldProgramId::ParticlesTranslucent || id == WorldProgramId::Clouds ||
        id == WorldProgramId::Weather || id == WorldProgramId::Hand || id == WorldProgramId::DamagedBlock;
}
}
