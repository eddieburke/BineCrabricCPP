#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

namespace net::minecraft {
class World;
class ChunkSource;
struct DimensionType;

// A live dimension instance: a thin runtime over a registered DimensionType
// (its data + generator/biome/fog/spawn seams). All per-type behaviour comes
// from that definition, so adding a dimension never touches this class — a mod
// just calls dimension::registerDimension and the player can travel into it.
class Dimension {
   public:
    World* world = nullptr;
    std::unique_ptr<BiomeSource> biomeSource;
    bool isNether = false;
    bool evaporatesWater = false;
    bool hasCeiling = false;
    std::array<float, 16> lightLevelToLuminance{};
    int id = 0;
    explicit Dimension(const DimensionType& type);
    void setWorld(World* worldIn);
    [[nodiscard]] static float luminanceForLightLevel(int lightLevel);
    void initBrightnessTable();
    void initBiomeSource();
    [[nodiscard]] std::unique_ptr<ChunkSource> createChunkGenerator();
    // Create a fresh generator with the given seed for a worker thread.
    [[nodiscard]] std::unique_ptr<ChunkSource> createChunkGeneratorFromSeed(std::uint64_t seed);

    // Save subfolder for this dimension's region/chunk files, relative to the world
    // root. Overworld (id 0) lives in the root (""); others use "DIM<id>".
    [[nodiscard]] std::string saveFolder() const {
        return id == 0 ? std::string() : "DIM" + std::to_string(id);
    }

    // Coordinate divisor relative to the overworld (Nether = 8.0 -> 8:1 compression).
    [[nodiscard]] double movementFactor() const;
    [[nodiscard]] bool isValidSpawnPoint(int x, int z) const;
    [[nodiscard]] float getTimeOfDay(long long time, float tickDelta) const;
    [[nodiscard]] std::array<float, 4>* getBackgroundColor(float timeOfDay, float tickDelta);
    [[nodiscard]] Vec3d getFogColor(float timeOfDay, float tickDelta) const;
    [[nodiscard]] bool hasWorldSpawn() const;
    // Build a dimension for a registered id, or null if none is registered.
    [[nodiscard]] static std::unique_ptr<Dimension> fromId(int dimensionId);
    [[nodiscard]] float getCloudHeight() const;
    [[nodiscard]] bool hasGround() const;

   private:
    // The definition that supplies generator/biome/environment.
    [[nodiscard]] const DimensionType& resolvedType() const;
    const DimensionType* type_ = nullptr;
    std::array<float, 4> backgroundColor_{};
};
}  // namespace net::minecraft
