#pragma once

#include "net/minecraft/world/chunk/Chunk.hpp"

#include <cstdint>
#include <vector>

namespace net::minecraft {

class NbtCompound;
class World;

// Direct binary NBT writer for Alpha/Beta chunk "Level" payloads. Skips the variant
// tree for bulk byte arrays while keeping wire-format parity with Java NbtIo.
class AlphaChunkNbtCodec {
public:
    static void writeRootChunk(std::vector<std::uint8_t>& out, Chunk& chunk, World* world);

    // Moves bulk arrays out of parsed NBT when possible to avoid extra copies on load.
    static Chunk loadChunkFromNbt(World* world, NbtCompound& nbt);
};

} // namespace net::minecraft
