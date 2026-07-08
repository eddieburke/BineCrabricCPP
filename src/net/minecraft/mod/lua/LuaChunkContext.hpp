#pragma once
#include <cstdint>

namespace net::minecraft {
class Chunk;
class World;
}  // namespace net::minecraft

namespace net::minecraft::mod::lua {
enum class ChunkWriteMode {
    ChunkApi = 0,
    RawGeneration = 1,
};

class LuaChunkContext {
   public:
    class Scope {
       public:
        Scope(Chunk* chunk, World* world, int chunkX, int chunkZ, ChunkWriteMode mode);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };

    [[nodiscard]] static bool hasActiveChunk() noexcept;
    [[nodiscard]] static Chunk* activeChunk() noexcept;
    [[nodiscard]] static World* activeWorld() noexcept;
    [[nodiscard]] static int activeChunkX() noexcept;
    [[nodiscard]] static int activeChunkZ() noexcept;
    [[nodiscard]] static ChunkWriteMode writeMode() noexcept;
    static bool setBlock(int localX, int y, int localZ, int blockId);
    [[nodiscard]] static int getBlock(int localX, int y, int localZ);
    [[nodiscard]] static int getHeight(int localX, int localZ);
};
}  // namespace net::minecraft::mod::lua
