#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
namespace net::minecraft::entity::decoration::painting {
struct PaintingVariant {
 const char* id = nullptr;
 int width = 64;
 int height = 64;
 int textureOffsetX = 0;
 int textureOffsetY = 0;
};
inline constexpr std::array<PaintingVariant, 25> PAINTING_VARIANTS{{
    {"Kebab", 16, 16, 0, 0},
    {"Aztec", 16, 16, 16, 0},
    {"Alban", 16, 16, 32, 0},
    {"Aztec2", 16, 16, 48, 0},
    {"Bomb", 16, 16, 64, 0},
    {"Plant", 16, 16, 80, 0},
    {"Wasteland", 16, 16, 96, 0},
    {"Pool", 32, 16, 0, 32},
    {"Courbet", 32, 16, 32, 32},
    {"Sea", 32, 16, 64, 32},
    {"Sunset", 32, 16, 96, 32},
    {"Creebet", 32, 16, 128, 32},
    {"Wanderer", 16, 32, 0, 64},
    {"Graham", 16, 32, 16, 64},
    {"Match", 32, 32, 0, 128},
    {"Bust", 32, 32, 32, 128},
    {"Stage", 32, 32, 64, 128},
    {"Void", 32, 32, 96, 128},
    {"SkullAndRoses", 32, 32, 128, 128},
    {"Fighters", 64, 32, 0, 96},
    {"Pointer", 64, 64, 0, 192},
    {"Pigscene", 64, 64, 64, 192},
    {"BurningSkull", 64, 64, 128, 192},
    {"Skeleton", 64, 48, 192, 64},
    {"DonkeyKong", 64, 48, 192, 112},
}};
inline constexpr PaintingVariant KEBAB = PAINTING_VARIANTS[0];
[[nodiscard]] inline const PaintingVariant& paintingVariantById(std::string_view id) {
 for(const PaintingVariant& variant : PAINTING_VARIANTS) {
  if(id == variant.id) {
   return variant;
  }
 }
 return KEBAB;
}
} // namespace net::minecraft::entity::decoration::painting
