#pragma once

#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft {

class NetherDimension : public Dimension {
public:
    NetherDimension()
    {
        id = -1;
        isNether = true;
        evaporatesWater = true;
        hasCeiling = true;
    }

    [[nodiscard]] double movementFactor() const override { return 8.0; }

    void initBiomeSource() override;
    void initBrightnessTable() override;
    [[nodiscard]] std::unique_ptr<ChunkSource> createChunkGenerator() override;
    [[nodiscard]] bool isValidSpawnPoint(int x, int z) const override;
    [[nodiscard]] float getTimeOfDay(long long time, float tickDelta) const override;
    [[nodiscard]] bool hasWorldSpawn() const override { return false; }
    [[nodiscard]] Vec3d getFogColor(float timeOfDay, float tickDelta) const override;
};

} // namespace net::minecraft
