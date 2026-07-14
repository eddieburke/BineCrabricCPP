// Built-in world types ported 1:1 from the former Dimension subclass hierarchy.
// Generators, biome sources, fog and spawn rules keep byte-identical math — only
// the plumbing moved into DimensionType data + seams. Registered lazily on first
// dimension lookup (see DimensionType.cpp), so mods may register before or after.
#include <algorithm>
#include <cstdint>
#include <memory>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/biome/source/FixedBiomeSource.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/DimensionType.hpp"
#include "net/minecraft/world/gen/chunk/NetherChunkGenerator.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#include "net/minecraft/world/gen/chunk/SkyChunkGenerator.hpp"
namespace net::minecraft::dimension {
namespace {
// Shared overworld/sky fog: brightness-modulated base colour. tintRgb<0 keeps the
// overworld's float literals; otherwise the rgb is unpacked (skylands tint).
Vec3d brightnessFog(float timeOfDay, int tintRgb) {
  float brightness = MathHelper::cos(timeOfDay * 3.14159265f * 2.0f) * 2.0f + 0.5f;
  brightness = std::clamp(brightness, 0.0f, 1.0f);
  float red = 0.7529412f;
  float green = 0.84705883f;
  float blue = 1.0f;
  if(tintRgb >= 0) {
    red = static_cast<float>((tintRgb >> 16) & 0xFF) / 255.0f;
    green = static_cast<float>((tintRgb >> 8) & 0xFF) / 255.0f;
    blue = static_cast<float>(tintRgb & 0xFF) / 255.0f;
  }
  red *= brightness * 0.94f + 0.06f;
  green *= brightness * 0.94f + 0.06f;
  blue *= brightness * 0.91f + 0.09f;
  return {red, green, blue};
}
} // namespace
void registerBuiltinDimensions() {
  // --- Overworld (id 0, also the "Default" world type) ---
  DimensionType overworld;
  overworld.id = 0;
  overworld.name = "Overworld";
  overworld.makeGenerator = [](World* world, std::uint64_t seed, bool localBiomeSource) {
    auto generator = std::make_unique<OverworldChunkGenerator>(world, seed);
    generator->useLocalBiomeSource(localBiomeSource);
    return std::unique_ptr<ChunkSource>(std::move(generator));
  };
  overworld.makeBiomeSource = [](World* world) -> std::unique_ptr<BiomeSource> {
    return std::make_unique<BiomeSource>(world->getSeed());
  };
  overworld.fogColor = [](float timeOfDay) { return brightnessFog(timeOfDay, -1); };
  overworld.isValidSpawn = [](World& world, int x, int z) {
    return Block::SAND != nullptr && world.getSpawnBlockId(x, z) == Block::SAND->id;
  };
  registerDimension(std::move(overworld));
  // --- Nether (id -1) ---
  DimensionType nether;
  nether.id = -1;
  nether.name = "Nether";
  nether.isNether = true;
  nether.evaporatesWater = true;
  nether.hasCeiling = true;
  nether.hasWorldSpawn = false;
  nether.fixedTime = true;
  nether.fixedTimeOfDay = 0.5f;
  nether.brightnessFactor = 0.1f;
  nether.movementFactor = 8.0;
  nether.makeGenerator = [](World* world, std::uint64_t seed, bool /*localBiomeSource*/) {
    return std::unique_ptr<ChunkSource>(std::make_unique<NetherChunkGenerator>(world, seed));
  };
  nether.makeBiomeSource = [](World* /*world*/) -> std::unique_ptr<BiomeSource> {
    return std::make_unique<FixedBiomeSource>(Biome::hell(), 1.0, 0.0);
  };
  nether.fogColor = [](float /*timeOfDay*/) { return Vec3d{0.2, 0.03, 0.03}; };
  nether.isValidSpawn = [](World& world, int x, int z) {
    const int spawnId = world.getSpawnBlockId(x, z);
    if(Block::BEDROCK != nullptr && spawnId == Block::BEDROCK->id) {
      return false;
    }
    if(spawnId == 0) {
      return false;
    }
    if(spawnId < 0 || spawnId >= static_cast<int>(Block::BLOCKS.size())) {
      return false;
    }
    return Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(spawnId)] != 0;
  };
  registerDimension(std::move(nether));
  // --- Skylands (id 1) ---
  DimensionType sky;
  sky.id = 1;
  sky.name = "Skylands";
  sky.hasGround = false;
  sky.cloudHeight = 8.0f;
  sky.hasBackgroundColor = false;
  sky.fixedTime = true;
  sky.fixedTimeOfDay = 0.0f;
  sky.makeGenerator = [](World* world, std::uint64_t seed, bool localBiomeSource) {
    auto generator = std::make_unique<SkyChunkGenerator>(world, seed);
    generator->useLocalBiomeSource(localBiomeSource);
    return std::unique_ptr<ChunkSource>(std::move(generator));
  };
  sky.makeBiomeSource = [](World* /*world*/) -> std::unique_ptr<BiomeSource> {
    return std::make_unique<FixedBiomeSource>(Biome::sky(), 0.5, 0.0);
  };
  sky.fogColor = [](float timeOfDay) { return brightnessFog(timeOfDay, 0x8080A0); };
  sky.isValidSpawn = [](World& world, int x, int z) {
    const int spawnId = world.getSpawnBlockId(x, z);
    if(spawnId == 0) {
      return false;
    }
    if(spawnId < 0 || spawnId >= static_cast<int>(Block::BLOCKS.size())) {
      return false;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(spawnId)];
    return block != nullptr && block->material.blocksMovement();
  };
  registerDimension(std::move(sky));
}
} // namespace net::minecraft::dimension
