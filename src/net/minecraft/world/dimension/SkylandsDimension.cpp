#include "net/minecraft/world/dimension/SkylandsDimension.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/FixedBiomeSource.hpp"
#include "net/minecraft/world/gen/chunk/SkyChunkGenerator.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft {

void SkylandsDimension::initBiomeSource()
{
    biomeSource = std::make_unique<FixedBiomeSource>(Biome::sky(), 0.5, 0.0);
    id = 1;
}

std::unique_ptr<ChunkSource> SkylandsDimension::createChunkGenerator()
{
    if (world == nullptr) {
        return nullptr;
    }
    return std::make_unique<SkyChunkGenerator>(world, world->seed());
}

std::unique_ptr<ChunkSource> SkylandsDimension::createChunkGeneratorFromSeed(std::uint64_t seed)
{
    if (world == nullptr) {
        return nullptr;
    }
    auto generator = std::make_unique<SkyChunkGenerator>(world, seed);
    generator->useLocalBiomeSource(true);
    return generator;
}

float SkylandsDimension::getTimeOfDay(long long time, float tickDelta) const
{
    (void)time;
    (void)tickDelta;
    return 0.0f;
}

Vec3d SkylandsDimension::getFogColor(float timeOfDay, float tickDelta) const
{
    (void)tickDelta;
    constexpr int fogRgb = 0x8080A0;
    float brightness = MathHelper::cos(timeOfDay * 3.14159265f * 2.0f) * 2.0f + 0.5f;
    brightness = std::clamp(brightness, 0.0f, 1.0f);
    float red = static_cast<float>((fogRgb >> 16) & 0xFF) / 255.0f;
    float green = static_cast<float>((fogRgb >> 8) & 0xFF) / 255.0f;
    float blue = static_cast<float>(fogRgb & 0xFF) / 255.0f;
    red *= brightness * 0.94f + 0.06f;
    green *= brightness * 0.94f + 0.06f;
    blue *= brightness * 0.91f + 0.09f;
    return {red, green, blue};
}

bool SkylandsDimension::isValidSpawnPoint(int x, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const int blockId = world->getSpawnBlockId(x, z);
    if (blockId == 0) {
        return false;
    }
    if (blockId < 0 || blockId >= static_cast<int>(Block::BLOCKS.size())) {
        return false;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    return block != nullptr && block->material.blocksMovement();
}

} // namespace net::minecraft
