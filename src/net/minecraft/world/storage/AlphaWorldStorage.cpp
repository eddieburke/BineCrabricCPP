#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/storage/exception/SessionLockException.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace net::minecraft {

namespace {

[[nodiscard]] std::uint64_t nowMillis()
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

} // namespace

AlphaWorldStorage::AlphaWorldStorage(fs::path savesDir, std::string name, bool createPlayerDataDir)
    : dir_(std::move(savesDir) / std::move(name)),
      playerDataDir_(dir_ / "players"),
      dataDir_(dir_ / "data"),
      startTime_(nowMillis())
{
    fs::create_directories(dir_);
    fs::create_directories(dataDir_);
    if (createPlayerDataDir) {
        fs::create_directories(playerDataDir_);
    }

    const fs::path lockFile = dir_ / "session.lock";
    std::ofstream output(lockFile, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Failed to check session lock, aborting");
    }
    std::vector<std::uint8_t> bytes;
    binary::appendI64BE(bytes, static_cast<std::int64_t>(startTime_));
    binary::writeAllBytes(output, bytes);
}

void AlphaWorldStorage::checkSessionLock()
{
    const fs::path lockFile = dir_ / "session.lock";
    std::ifstream input(lockFile, std::ios::binary);
    if (!input) {
        throw SessionLockException("Failed to check session lock, aborting");
    }

    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    const std::uint64_t stored = static_cast<std::uint64_t>(binary::readI64BE(bytes, pos));
    if (stored != startTime_) {
        throw SessionLockException("The save is being accessed from another location, aborting");
    }
}

std::unique_ptr<ChunkStorage> AlphaWorldStorage::getChunkStorage(const Dimension* dimension)
{
    const std::string sub = dimension != nullptr ? dimension->saveFolder() : std::string();
    const fs::path chunkDir = sub.empty() ? dir_ : dir_ / sub;
    fs::create_directories(chunkDir);
    return std::make_unique<AlphaChunkStorage>(chunkDir, true);
}

std::optional<WorldProperties> AlphaWorldStorage::loadProperties()
{
    if (const std::optional<WorldProperties> loaded = loadPropertiesFrom(dir_ / "level.dat"); loaded.has_value()) {
        return loaded;
    }
    return loadPropertiesFrom(dir_ / "level.dat_old");
}

std::optional<WorldProperties> AlphaWorldStorage::loadPropertiesFrom(const fs::path& file)
{
    if (!fs::exists(file)) {
        return std::nullopt;
    }

    std::ifstream input(file, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }

    try {
        const NbtCompound root = NbtIo::readCompressed(input);
        if (!root.contains("Data")) {
            return std::nullopt;
        }
        return WorldProperties(root.getCompound("Data"));
    } catch (...) {
        return std::nullopt;
    }
}

void AlphaWorldStorage::writeLevelDat(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players)
{
    NbtCompound root;
    root.put("Data", properties.asNbt(players));

    const fs::path temp = dir_ / "level.dat_new";
    const fs::path backup = dir_ / "level.dat_old";
    const fs::path current = dir_ / "level.dat";

    {
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Failed to open temporary level.dat file");
        }
        NbtIo::writeCompressed(root, output);
    }

    std::error_code ec;
    fs::remove(backup, ec);
    ec.clear();
    fs::rename(current, backup, ec);
    ec.clear();
    fs::remove(current, ec);
    ec.clear();
    fs::rename(temp, current, ec);
    ec.clear();
    fs::remove(temp, ec);
}

void AlphaWorldStorage::save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players)
{
    try {
        writeLevelDat(properties, players);
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save level data: " << exception.what() << '\n';
    }
}

void AlphaWorldStorage::save(const WorldProperties& properties)
{
    save(properties, {});
}

void AlphaWorldStorage::savePlayerData(entity::player::PlayerEntity& player)
{
    fs::create_directories(playerDataDir_);
    const fs::path temp = playerDataDir_ / "_tmp_.dat";
    const fs::path file = playerDataDir_ / (player.name + ".dat");

    try {
        NbtCompound nbt;
        player.writeNbt(nbt);
        {
            std::ofstream output(temp, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("Failed to open temporary player file");
            }
            NbtIo::writeCompressed(nbt, output);
        }

        std::error_code ec;
        fs::remove(file, ec);
        ec.clear();
        fs::rename(temp, file, ec);
        ec.clear();
        fs::remove(temp, ec);
    } catch (...) {
        std::cerr << "Failed to save player data for " << player.name << '\n';
        std::error_code ec;
        fs::remove(temp, ec);
    }
}

void AlphaWorldStorage::loadPlayerData(entity::player::PlayerEntity& player)
{
    if (const Nbt nbt = loadPlayerData(player.name); nbt.type() == Nbt::Type::Compound) {
        player.readNbt(NbtCompound(nbt));
    }
}

Nbt AlphaWorldStorage::loadPlayerData(const std::string& playerName)
{
    const fs::path file = playerDataDir_ / (playerName + ".dat");
    if (!fs::exists(file)) {
        return {};
    }

    try {
        std::ifstream input(file, std::ios::binary);
        if (!input) {
            return {};
        }
        return NbtIo::readCompressed(input).storage();
    } catch (...) {
        std::cerr << "Failed to load player data for " << playerName << '\n';
        return {};
    }
}

fs::path AlphaWorldStorage::getWorldPropertiesFile(const std::string& name) const
{
    return dataDir_ / (name + ".dat");
}

} // namespace net::minecraft
