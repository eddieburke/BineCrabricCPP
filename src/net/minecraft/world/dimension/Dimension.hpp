#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <string>

namespace net::minecraft {

class World;
class ChunkSource;

class Dimension {
public:
    virtual ~Dimension() = default;

    World* world = nullptr;
    std::unique_ptr<BiomeSource> biomeSource;
    bool isNether = false;
    bool evaporatesWater = false;
    bool hasCeiling = false;
    std::array<float, 16> lightLevelToLuminance {};
    int id = 0;

    void setWorld(World* worldIn);
    [[nodiscard]] static float luminanceForLightLevel(int lightLevel);
    virtual void initBrightnessTable();
    virtual void initBiomeSource();
    [[nodiscard]] virtual std::unique_ptr<ChunkSource> createChunkGenerator();

    // Create a fresh generator with the given seed for a worker thread.
    [[nodiscard]] virtual std::unique_ptr<ChunkSource> createChunkGeneratorFromSeed(std::uint64_t seed);

    // Save subfolder for this dimension's region/chunk files, relative to the world
    // root. Overworld (id 0) lives in the root (""); others use "DIM<id>" — the
    // Nether (-1) -> "DIM-1", Sky (1) -> "DIM1". Single source of truth for the
    // per-dimension region split (was duplicated, and inconsistent, in the storage
    // classes: one used dynamic_cast<NetherDimension*>, the other id == -1).
    [[nodiscard]] virtual std::string saveFolder() const
    {
        return id == 0 ? std::string() : "DIM" + std::to_string(id);
    }

    // Coordinate divisor relative to the overworld (Nether = 8.0 -> 8:1 compression).
    // Travel scales positions by source.movementFactor() / target.movementFactor().
    [[nodiscard]] virtual double movementFactor() const { return 1.0; }

    [[nodiscard]] virtual bool isValidSpawnPoint(int x, int z) const;

    [[nodiscard]] virtual float getTimeOfDay(long long time, float tickDelta) const
    {
        const int timeOfDay = static_cast<int>(time % 24000LL);
        float value = (static_cast<float>(timeOfDay) + tickDelta) / 24000.0f - 0.25f;
        if (value < 0.0f) {
            value += 1.0f;
        }
        if (value > 1.0f) {
            value -= 1.0f;
        }
        const float original = value;
        value = 1.0f - static_cast<float>((std::cos(static_cast<double>(value) * 3.14159265) + 1.0) / 2.0);
        return original + (value - original) / 3.0f;
    }

    [[nodiscard]] virtual std::array<float, 4>* getBackgroundColor(float timeOfDay, float)
    {
        constexpr float range = 0.4f;
        const float cosine = MathHelper::cos(timeOfDay * 3.14159265f * 2.0f) - 0.0f;
        if (cosine >= -range && cosine <= range) {
            const float blend = (cosine - -0.0f) / range * 0.5f + 0.5f;
            float alpha = 1.0f - (1.0f - MathHelper::sin(blend * 3.14159265f)) * 0.99f;
            alpha *= alpha;
            backgroundColor_[0] = blend * 0.3f + 0.7f;
            backgroundColor_[1] = blend * blend * 0.7f + 0.2f;
            backgroundColor_[2] = blend * blend * 0.0f + 0.2f;
            backgroundColor_[3] = alpha;
            return &backgroundColor_;
        }
        return nullptr;
    }

    [[nodiscard]] virtual Vec3d getFogColor(float timeOfDay, float) const
    {
        float brightness = MathHelper::cos(timeOfDay * 3.14159265f * 2.0f) * 2.0f + 0.5f;
        brightness = std::clamp(brightness, 0.0f, 1.0f);
        float red = 0.7529412f;
        float green = 0.84705883f;
        float blue = 1.0f;
        red *= brightness * 0.94f + 0.06f;
        green *= brightness * 0.94f + 0.06f;
        blue *= brightness * 0.91f + 0.09f;
        return {red, green, blue};
    }

    [[nodiscard]] virtual bool hasWorldSpawn() const
    {
        return true;
    }

    [[nodiscard]] static std::unique_ptr<Dimension> fromId(int dimensionId);

    [[nodiscard]] virtual float getCloudHeight() const
    {
        return 108.0f;
    }

    [[nodiscard]] virtual bool hasGround() const
    {
        return true;
    }

protected:
    std::array<float, 4> backgroundColor_ {};
};

} // namespace net::minecraft
