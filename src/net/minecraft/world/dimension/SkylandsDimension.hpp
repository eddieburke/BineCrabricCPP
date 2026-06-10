#pragma once

#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft {

class SkylandsDimension : public Dimension {
public:
    SkylandsDimension()
    {
        id = 1;
    }

    void initBiomeSource() override;
    [[nodiscard]] std::array<float, 4>* getBackgroundColor(float, float) override { return nullptr; }
    [[nodiscard]] std::unique_ptr<ChunkSource> createChunkGenerator() override;
    [[nodiscard]] std::unique_ptr<ChunkSource> createChunkGeneratorFromSeed(std::uint64_t seed) override;
    [[nodiscard]] float getTimeOfDay(long long time, float tickDelta) const override;
    [[nodiscard]] Vec3d getFogColor(float timeOfDay, float tickDelta) const override;
    [[nodiscard]] bool hasGround() const override { return false; }
    [[nodiscard]] float getCloudHeight() const override { return 8.0f; }
    [[nodiscard]] bool isValidSpawnPoint(int x, int z) const override;
};

} // namespace net::minecraft
