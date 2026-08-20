#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include "net/minecraft/world/biome/Biome.hpp"

namespace net::minecraft::client::render {
























inline constexpr std::array<std::string_view, kBiomeCount> kBiomeNames = {
    "RAINFOREST", "SWAMP", "SEASONAL_FOREST", "FOREST", "SAVANNA", "SHRUBLAND", "TAIGA",
    "DESERT", "PLAINS", "ICE_DESERT", "TUNDRA", "NETHER_WASTES", "THE_END"};

enum class BiomeCategory : std::size_t {
 None = 0,
 Taiga = 1,
 ExtremeHills = 2,
 Jungle = 3,
 Mesa = 4,
 Plains = 5,
 Savanna = 6,
 Icy = 7,
 TheEnd = 8,
 Beach = 9,
 Forest = 10,
 Ocean = 11,
 Desert = 12,
 River = 13,
 Swamp = 14,
 Mushroom = 15,
 Nether = 16,
 Mountain = 17,
 Underground = 18,
};
inline constexpr std::size_t kBiomeCategoryCount = 19;
inline constexpr std::array<std::string_view, kBiomeCategoryCount> kBiomeCategoryNames = {
    "NONE", "TAIGA", "EXTREME_HILLS", "JUNGLE", "MESA", "PLAINS", "SAVANNA", "ICY", "THE_END",
    "BEACH", "FOREST", "OCEAN", "DESERT", "RIVER", "SWAMP", "MUSHROOM", "NETHER", "MOUNTAIN",
    "UNDERGROUND"};
static_assert(kBiomeCategoryNames.size() == kBiomeCategoryCount);
static_assert(kBiomeNames.size() == static_cast<std::size_t>(BiomeId::Sky) + 1);



struct BiomeNameAlias {
 std::string_view name;
 std::size_t index;
};
inline constexpr std::array<BiomeNameAlias, 5> kBiomeNameAliases = {
    {{"SWAMPLAND", 1}, {"DARK_FOREST", 3}, {"HELL", 11}, {"NETHER", 11}, {"SKY", 12}}};






struct BiomeClimate {
 BiomeCategory category;
 float temperature;
 float rainfall;
};
inline constexpr std::array<BiomeClimate, kBiomeCount> kBiomeClimates = {
    BiomeClimate{BiomeCategory::Jungle, 0.95f, 0.9f},    
    BiomeClimate{BiomeCategory::Swamp, 0.8f, 0.9f},      
    BiomeClimate{BiomeCategory::Forest, 0.7f, 0.8f},     
    BiomeClimate{BiomeCategory::Forest, 0.7f, 0.8f},     
    BiomeClimate{BiomeCategory::Savanna, 0.8f, 0.4f},    
    BiomeClimate{BiomeCategory::Savanna, 0.8f, 0.4f},    
    BiomeClimate{BiomeCategory::Taiga, 0.25f, 0.8f},     
    BiomeClimate{BiomeCategory::Desert, 2.0f, 0.0f},     
    BiomeClimate{BiomeCategory::Plains, 0.8f, 0.4f},     
    BiomeClimate{BiomeCategory::Desert, 0.0f, 0.0f},     
    BiomeClimate{BiomeCategory::Icy, 0.0f, 0.5f},        
    BiomeClimate{BiomeCategory::Nether, 2.0f, 0.0f},     
    BiomeClimate{BiomeCategory::TheEnd, 0.5f, 0.0f},     
};
static_assert(kBiomeClimates.size() == kBiomeCount);
} 
