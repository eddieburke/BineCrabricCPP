#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft {
class World;
class ChunkSource;
class BiomeSource;

// A registered world/dimension definition. *Everything* that differs between
// world types is plain data here — including the chunk generator and biome
// source, which are std::function seams a mod fills with its OWN types (zero
// dependence on the built-in generators). Adding a world type = fill one of
// these and call dimension::registerDimension(); no subclass, no core edits.
//
// fogColor / isValidSpawn are seams too, so a mod sets its own sky/spawn rules.
// The remaining fields (brightness, time, cloud, movement, flags) are consumed
// by the shared formulas in Dimension.
struct DimensionType {
    int id = 0;
    std::string name = "Overworld";  // display name
    bool isNether = false;
    bool evaporatesWater = false;
    bool hasCeiling = false;
    bool hasGround = true;
    bool hasWorldSpawn = true;
    bool hasBackgroundColor = true;
    bool fixedTime = false;
    float fixedTimeOfDay = 0.0f;
    float brightnessFactor = 0.05f;
    double movementFactor = 1.0;
    float cloudHeight = 108.0f;
    // Required seams. localBiomeSource mirrors the worker-thread flag the
    // built-in generators honour; pass it straight through to your generator.
    std::function<std::unique_ptr<ChunkSource>(World*, std::uint64_t seed, bool localBiomeSource)> makeGenerator;
    std::function<std::unique_ptr<BiomeSource>(World*)> makeBiomeSource;
    std::function<Vec3d(float timeOfDay)> fogColor;
    std::function<bool(World&, int x, int z)> isValidSpawn;
};

namespace dimension {
// Append (or, for an existing id, replace) a definition. Call from a mod's
// static initializer or any time before the target world is created. Built-in
// ids (-1/0/1) are registered lazily on first lookup, so a mod may register
// before or after them freely.
void registerDimension(DimensionType type);
[[nodiscard]] const DimensionType* dimensionById(int id);
[[nodiscard]] const std::vector<const DimensionType*>& allDimensions();
// Defined in VanillaDimensions.cpp; invoked once by the lookups above.
void registerBuiltinDimensions();
}  // namespace dimension
}  // namespace net::minecraft
