#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"

#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkNbtCodec.hpp"
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

        NbtCompound root = NbtIo::readCompressed(input);
        return loadChunkFromRootNbt(world, root, chunkX, chunkZ);
    } catch (const std::exception& exception) {
        std::cout << "Failed to load chunk at " << chunkX << "," << chunkZ << ": " << exception.what() << '\n';
        return EmptyChunk(world, chunkX, chunkZ);
    }
}

Chunk AlphaChunkStorage::loadChunkFromRootNbt(World* world, NbtCompound& root, int chunkX, int chunkZ)
{
    if (!root.contains("Level")) {
        std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing level data, skipping\n";
        return EmptyChunk(world, chunkX, chunkZ);
    }

    NbtCompound levelCompound = root.getCompound("Level");
    if (!levelCompound.contains("Blocks")) {
        std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is missing block data, skipping\n";
        return EmptyChunk(world, chunkX, chunkZ);
    }

    if (levelCompound.getInt("xPos") != chunkX || levelCompound.getInt("zPos") != chunkZ) {
        std::cout << "Chunk file at " << chunkX << "," << chunkZ << " is in the wrong location; relocating. (Expected "
                  << chunkX << ", " << chunkZ << ", got " << levelCompound.getInt("xPos") << ", "
                  << levelCompound.getInt("zPos") << ")\n";
        levelCompound.putInt("xPos", chunkX);
        levelCompound.putInt("zPos", chunkZ);
    }

    Chunk chunk = AlphaChunkNbtCodec::loadChunkFromNbt(world, levelCompound);
    chunk.fill();
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
        std::vector<std::uint8_t> raw;
        AlphaChunkNbtCodec::writeRootChunk(raw, chunk, world);

        {
            std::ofstream output(temp, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("Failed to open temporary chunk file");
            }
            NbtIo::writeCompressed(raw, output);
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

} // namespace net::minecraft
