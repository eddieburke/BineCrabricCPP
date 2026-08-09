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
    "gbuffers_skytextured", "gbuffers_basic", "gbuffers_line", "gbuffers_entities",
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
