#pragma once

#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"

#include <filesystem>
#include <string>

namespace net::minecraft {

namespace fs = std::filesystem;

class AlphaChunkStorage : public ChunkStorage {
public:
    AlphaChunkStorage(fs::path dir, bool make);

    [[nodiscard]] fs::path getChunkFile(int chunkX, int chunkZ) const;

    Chunk loadChunk(World* world, int chunkX, int chunkZ) override;
    void saveChunk(World* world, Chunk& chunk) override;

    static NbtCompound saveChunkToNbt(Chunk& chunk, World* world, NbtCompound nbt);

    static Chunk loadChunkFromNbt(World* world, const NbtCompound& nbt);

    void tick() override {}
    void flush() override {}

    void saveEntities(World* world, Chunk& chunk) override
    {
        (void)world;
        (void)chunk;
    }

private:
    fs::path dir_;
    bool make_ = false;
};

} // namespace net::minecraft
