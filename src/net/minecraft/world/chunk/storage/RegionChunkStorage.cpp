#include "net/minecraft/world/chunk/storage/RegionChunkStorage.hpp"

#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/World.hpp"

#include <iostream>

namespace net::minecraft {

RegionChunkStorage::RegionChunkStorage(fs::path dir) : dir_(std::move(dir)) {}

Chunk RegionChunkStorage::loadChunk(World* world, int chunkX, int chunkZ)
{
    const std::optional<std::vector<std::uint8_t>> raw = RegionIo::readChunkData(dir_, chunkX, chunkZ);
    if (!raw.has_value()) {
        return EmptyChunk(world, chunkX, chunkZ);
    }

    try {
        const Nbt root = Nbt::read(*raw);
        const Nbt* level = root.get("Level");
        if (level == nullptr || !level->isCompound()) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing level data, skipping\n";
            return EmptyChunk(world, chunkX, chunkZ);
        }

        NbtCompound levelCompound(*level);
        if (!levelCompound.contains("Blocks")) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing block data, skipping\n";
            return EmptyChunk(world, chunkX, chunkZ);
        }

        Chunk chunk = AlphaChunkStorage::loadChunkFromNbt(world, levelCompound);
        if (!chunk.chunkPosEquals(chunkX, chunkZ)) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ
                      << " is in the wrong location; relocating. (Expected " << chunkX << ", " << chunkZ << ", got "
                      << chunk.x << ", " << chunk.z << ")\n";
            NbtCompound relocated = levelCompound;
            relocated.putInt("xPos", chunkX);
            relocated.putInt("zPos", chunkZ);
            Chunk relocatedChunk = AlphaChunkStorage::loadChunkFromNbt(world, relocated);
            relocatedChunk.world = world;
            relocatedChunk.fill();
            return relocatedChunk;
        }
        chunk.world = world;
        chunk.fill();
        return chunk;
    } catch (const std::exception& exception) {
        std::cout << "Failed to load chunk at " << chunkX << "," << chunkZ << ": " << exception.what() << '\n';
        return EmptyChunk(world, chunkX, chunkZ);
    }
}

void RegionChunkStorage::saveChunk(World* world, Chunk& chunk)
{
    if (world == nullptr) {
        return;
    }
    world->checkSessionLock();

    try {
        NbtCompound root;
        NbtCompound level;
        root.put("Level", level);
        AlphaChunkStorage::saveChunkToNbt(chunk, world, level);
        const std::vector<std::uint8_t> raw = root.toStorage().toBytes();
        RegionIo::writeChunkData(dir_, chunk.x, chunk.z, raw);

        WorldProperties& properties = world->getProperties();
        properties.setSizeOnDisk(properties.getSizeOnDisk()
            + static_cast<std::uint64_t>(RegionIo::getChunkSize(dir_, chunk.x, chunk.z)));
    } catch (const std::exception& exception) {
        std::cout << "Failed to save chunk at " << chunk.x << "," << chunk.z << ": " << exception.what() << '\n';
    }
}

void RegionChunkStorage::saveEntities(World* world, Chunk& chunk)
{
    saveChunk(world, chunk);
}

void RegionChunkStorage::tick() {}

void RegionChunkStorage::flush()
{
    RegionIo::flush();
}

} // namespace net::minecraft
