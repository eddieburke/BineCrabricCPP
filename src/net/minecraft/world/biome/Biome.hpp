#pragma once

#include "net/minecraft/world/biome/EntitySpawnGroup.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft {

class Feature;

enum class BiomeId {
    Rainforest,
    Swampland,
    SeasonalForest,
    Forest,
    Savanna,
    Shrubland,
    Taiga,
    Desert,
    Plains,
    IceDesert,
    Tundra,
    Hell,
    Sky,
};

inline constexpr int kBiomeCount = 13;
inline constexpr std::uint8_t kBiomeDefaultTopBlockId = 2;
inline constexpr std::uint8_t kBiomeDefaultSoilBlockId = 3;
inline constexpr std::uint8_t kBiomeSandSurfaceBlockId = 12;

// Faithful port of net.minecraft.world.biome.Biome (beta 1.7.3).
class Biome {
public:
    Biome();

    Biome& setName(std::string value) { name = std::move(value); return *this; }
    Biome& setGrassColor(int value) { grassColor = value; return *this; }
    Biome& setFoliageColor(int value) { foliageColor = value; return *this; }
    Biome& enableSnow() { hasSnow = true; return *this; }
    Biome& disableRain() { hasRain = false; return *this; }

    [[nodiscard]] virtual std::unique_ptr<Feature> getRandomTreeFeature(JavaRandom& random) const;
    [[nodiscard]] virtual int getSkyColor(float brightness) const;

    [[nodiscard]] const std::vector<EntitySpawnGroup>& getSpawnableEntities(int group) const;
    [[nodiscard]] bool canSnow() const noexcept { return hasSnow; }
    [[nodiscard]] bool canRain() const noexcept { return hasRain && !hasSnow; }
    [[nodiscard]] std::string_view wireName() const noexcept { return wireName_; }

    static void init();

    static Biome& rainforest();
    static Biome& swampland();
    static Biome& seasonalForest();
    static Biome& forest();
    static Biome& savanna();
    static Biome& shrubland();
    static Biome& taiga();
    static Biome& desert();
    static Biome& plains();
    static Biome& iceDesert();
    static Biome& tundra();
    static Biome& hell();
    static Biome& sky();

    [[nodiscard]] static Biome& byId(BiomeId id);
    [[nodiscard]] static Biome& getBiome(double temperature, double downfall);
    [[nodiscard]] static Biome& locateBiome(float temperature, float downfall);

    BiomeId id = BiomeId::Plains;
    std::string name = "Plains";
    int grassColor = 0;
    std::uint8_t topBlockId = kBiomeDefaultTopBlockId;
    std::uint8_t soilBlockId = kBiomeDefaultSoilBlockId;
    int foliageColor = 5169201;

protected:
    std::vector<EntitySpawnGroup> spawnableMonsters_;
    std::vector<EntitySpawnGroup> spawnablePassive_;
    std::vector<EntitySpawnGroup> spawnableWaterCreatures_;

private:
    bool hasSnow = false;
    bool hasRain = true;
    std::string_view wireName_ = "plains";
};

} // namespace net::minecraft
