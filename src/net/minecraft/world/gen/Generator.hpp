#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <cstdint>

namespace net::minecraft {

class ChunkSource;
class World;

// Faithful to Java net.minecraft.world.gen.Generator. The Java carver mutates the
// chunk's raw byte[]; here we operate directly on Chunk::blocks (index x<<11|z<<7|y,
// identical to Java's (x*16+z)*128+y) so writes bypass heightmap/meta side effects,
// matching the decompiled behavior.
class Generator {
public:
    virtual ~Generator() = default;

    void place(ChunkSource* source, World* world, std::uint64_t worldSeed, int chunkX, int chunkZ, Chunk& chunk);

    // Raw block accessors on the chunk's storage, matching the Java carver's direct
    // byte[] indexing (no heightmap recompute, no metadata change).
    [[nodiscard]] static int rawBlock(const Chunk& chunk, int localX, int y, int localZ)
    {
        return static_cast<int>(chunk.blocks[static_cast<std::size_t>((localX << 11) | (localZ << 7) | y)] & 0xFFU);
    }

    static void setRawBlock(Chunk& chunk, int localX, int y, int localZ, int blockId)
    {
        chunk.blocks[static_cast<std::size_t>((localX << 11) | (localZ << 7) | y)] = static_cast<std::uint8_t>(blockId & 0xFF);
    }

protected:
    virtual void place(World* /*world*/, int /*startChunkX*/, int /*startChunkZ*/, int /*chunkX*/, int /*chunkZ*/, Chunk& /*chunk*/)
    {
    }

    int range = 8;
    JavaRandom random;
};

} // namespace net::minecraft
