#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"

#include <fstream>
#include <iostream>
#include <mutex>

namespace net::minecraft {

AlphaChunkStorage::AlphaChunkStorage(fs::path dir, bool make) : dir_(std::move(dir)), make_(make)
{
    if (make_) {
        fs::create_directories(dir_);
    }
}

namespace {

std::mutex& chunkFileMutex()
{
    static std::mutex mutex;
    return mutex;
}

[[nodiscard]] std::string toBase36(int value)
{
    if (value == 0) {
        return "0";
    }
    bool negative = value < 0;
    if (negative) {
        value = -value;
    }
    std::string out;
    while (value > 0) {
        const int digit = value % 36;
        out.push_back(static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10));
        value /= 36;
    }
    if (negative) {
        out.push_back('-');
    }
    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace

fs::path AlphaChunkStorage::getChunkFile(int chunkX, int chunkZ) const
{
    const std::string fileName = "c." + toBase36(chunkX) + "." + toBase36(chunkZ) + ".dat";
    const fs::path subX = dir_ / toBase36(chunkX & 0x3F);
    if (!fs::exists(subX)) {
        if (!make_) {
            return {};
        }
        fs::create_directories(subX);
    }

    const fs::path subZ = subX / toBase36(chunkZ & 0x3F);
    if (!fs::exists(subZ)) {
        if (!make_) {
            return {};
        }
        fs::create_directories(subZ);
    }

    const fs::path file = subZ / fileName;
    if (!fs::exists(file) && !make_) {
        return {};
    }
    return file;
}

Chunk AlphaChunkStorage::loadChunk(World* world, int chunkX, int chunkZ)
{
    const std::lock_guard lock(chunkFileMutex());
    const fs::path file = getChunkFile(chunkX, chunkZ);
    if (file.empty() || !fs::exists(file)) {
        return EmptyChunk(world, chunkX, chunkZ);
    }

    try {
        std::ifstream input(file, std::ios::binary);
        if (!input) {
            return EmptyChunk(world, chunkX, chunkZ);
        }

        const NbtCompound root = NbtIo::readCompressed(input);
        if (!root.contains("Level")) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing level data, skipping\n";
            return EmptyChunk(world, chunkX, chunkZ);
        }
        const NbtCompound level = root.getCompound("Level");
        if (!level.contains("Blocks")) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing block data, skipping\n";
            return EmptyChunk(world, chunkX, chunkZ);
        }

        Chunk chunk = loadChunkFromNbt(world, level);
        if (!chunk.chunkPosEquals(chunkX, chunkZ)) {
            std::cout << "Chunk file at " << chunkX << "," << chunkZ
                      << " is in the wrong location; relocating. (Expected " << chunkX << ", " << chunkZ << ", got "
                      << chunk.x << ", " << chunk.z << ")\n";
            NbtCompound relocated = level;
            relocated.putInt("xPos", chunkX);
            relocated.putInt("zPos", chunkZ);
            Chunk relocatedChunk = loadChunkFromNbt(world, relocated);
            relocatedChunk.fill();
            return relocatedChunk;
        }
        chunk.fill();
        return chunk;
    } catch (const std::exception& exception) {
        std::cout << "Failed to load chunk at " << chunkX << "," << chunkZ << ": " << exception.what() << '\n';
        return EmptyChunk(world, chunkX, chunkZ);
    }
}

Chunk AlphaChunkStorage::loadChunkFromNbt(World* world, const NbtCompound& nbt)
{
    initializeBlocks();
    Chunk chunk(world, nbt.getInt("xPos"), nbt.getInt("zPos"));

    const auto blockBytes = nbt.getByteArray("Blocks");
    std::copy_n(blockBytes.begin(), std::min(blockBytes.size(), chunk.blocks.size()), chunk.blocks.begin());
    chunk.meta = ChunkNibbleArray(nbt.getByteArray("Data"));
    const std::size_t expectedLightBytes = chunk.blocks.size() / 2;
    const auto skyBytes = nbt.getByteArray("SkyLight");
    chunk.skyLight = ChunkNibbleArray(skyBytes);
    chunk.blockLight = ChunkNibbleArray(nbt.getByteArray("BlockLight"));
    const auto heightBytes = nbt.getByteArray("HeightMap");
    if (!heightBytes.empty()) {
        std::copy_n(heightBytes.begin(), std::min(heightBytes.size(), chunk.heightmap.size()), chunk.heightmap.begin());
    }
    chunk.terrainPopulated = nbt.getBoolean("TerrainPopulated");

    if (!chunk.meta.isArrayInitialized()) {
        chunk.meta = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
    } else if (!chunk.meta.hasExpectedSizeForBlockCount(chunk.blocks.size())) {
        chunk.meta.ensureSizeForBlockCount(chunk.blocks.size());
    }

    const bool skyLightInitialized = chunk.skyLight.isArrayInitialized()
        && chunk.skyLight.bytes.size() == expectedLightBytes;
    if (heightBytes.empty() || !skyLightInitialized || chunk.skyLight.isAllZero()) {
        chunk.heightmap.fill(0);
        chunk.skyLight = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
        chunk.populateHeightMap();
    } else {
        // Rebuild heightmap from blocks so hasSkyLight() matches the saved terrain.
        // A stale saved heightmap makes outdoor plants fail PlantBlock survival checks.
        chunk.populateHeightMapOnly();
    }

    if (!chunk.blockLight.isArrayInitialized() || chunk.blockLight.bytes.size() != expectedLightBytes) {
        chunk.blockLight = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
        chunk.onLoad();
    }

    if (nbt.contains("Entities")) {
        const NbtList entities = nbt.getList("Entities");
        for (const Nbt& entry : entities.entries()) {
            if (!entry.isCompound()) {
                continue;
            }
            const NbtCompound entityNbt = NbtCompound(Nbt(entry));
            if (std::unique_ptr<Entity> entity = EntityRegistry::getEntityFromNbt(entityNbt, world)) {
                chunk.lastSaveHadEntities = true;
                chunk.addEntity(entity.release());
            }
        }
    }

    if (nbt.contains("TileEntities")) {
        const NbtList tileEntities = nbt.getList("TileEntities");
        for (const Nbt& entry : tileEntities.entries()) {
            if (!entry.isCompound()) {
                continue;
            }
            const NbtCompound tileNbt = NbtCompound(Nbt(entry));
            if (std::unique_ptr<block::entity::BlockEntity> blockEntity = block::entity::BlockEntity::createFromNbt(tileNbt)) {
                chunk.addBlockEntity(std::move(blockEntity));
            }
        }
    }

    return chunk;
}

void AlphaChunkStorage::saveChunk(World* world, Chunk& chunk)
{
    if (world == nullptr) {
        return;
    }
    const std::lock_guard lock(chunkFileMutex());
    world->checkSessionLock();

    const fs::path file = getChunkFile(chunk.x, chunk.z);
    if (file.empty()) {
        return;
    }

    WorldProperties& properties = world->getProperties();
    if (fs::exists(file)) {
        properties.setSizeOnDisk(properties.getSizeOnDisk() - static_cast<std::uint64_t>(fs::file_size(file)));
    }

    try {
        const fs::path temp = dir_ / "tmp_chunk.dat";
        NbtCompound root;
        NbtCompound level;
        root.put("Level", level);
        saveChunkToNbt(chunk, world, level);

        {
            std::ofstream output(temp, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("Failed to open temporary chunk file");
            }
            NbtIo::writeCompressed(root, output);
        }

        std::error_code ec;
        fs::remove(file, ec);
        ec.clear();
        fs::rename(temp, file, ec);
        ec.clear();
        fs::remove(temp, ec);

        if (fs::exists(file)) {
            properties.setSizeOnDisk(properties.getSizeOnDisk() + static_cast<std::uint64_t>(fs::file_size(file)));
        }
    } catch (const std::exception& exception) {
        std::cout << "Failed to save chunk at " << chunk.x << "," << chunk.z << ": " << exception.what() << '\n';
    }
}

NbtCompound AlphaChunkStorage::saveChunkToNbt(Chunk& chunk, World* world, NbtCompound nbt)
{
    if (world != nullptr) {
        world->checkSessionLock();
    }

    nbt.putInt("xPos", chunk.x);
    nbt.putInt("zPos", chunk.z);
    nbt.putLong("LastUpdate", static_cast<std::int64_t>(world != nullptr ? world->getTime() : 0));
    nbt.putByteArray("Blocks", std::vector<std::uint8_t>(chunk.blocks.begin(), chunk.blocks.end()));
    if (!chunk.meta.hasExpectedSizeForBlockCount(chunk.blocks.size())) {
        chunk.meta.ensureSizeForBlockCount(chunk.blocks.size());
    }
    nbt.putByteArray("Data", chunk.meta.bytes);
    nbt.putByteArray("SkyLight", chunk.skyLight.bytes);
    nbt.putByteArray("BlockLight", chunk.blockLight.bytes);
    nbt.putByteArray("HeightMap", std::vector<std::uint8_t>(chunk.heightmap.begin(), chunk.heightmap.end()));
    nbt.putBoolean("TerrainPopulated", chunk.terrainPopulated);
    chunk.lastSaveHadEntities = false;

    NbtList entities;
    for (const std::vector<Entity*>& slice : chunk.entities) {
        for (Entity* entity : slice) {
            if (entity == nullptr) {
                continue;
            }
            NbtCompound entityNbt;
            if (!entity->saveSelfNbt(entityNbt)) {
                continue;
            }
            chunk.lastSaveHadEntities = true;
            entities.storage().asList().push_back(entityNbt.storage());
        }
    }
    nbt.put("Entities", entities);

    NbtList tileEntities;
    for (const auto& entry : chunk.blockEntities) {
        if (entry.second == nullptr) {
            continue;
        }
        NbtCompound tileNbt;
        entry.second->writeNbt(tileNbt);
        tileEntities.storage().asList().push_back(tileNbt.storage());
    }
    nbt.put("TileEntities", tileEntities);
    return nbt;
}

} // namespace net::minecraft
