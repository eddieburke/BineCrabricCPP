#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "net/minecraft/nbt/NbtCompound.hpp"

namespace net::minecraft {
namespace entity::player {
class PlayerEntity;
}

class WorldProperties {
   public:
    WorldProperties() = default;

    WorldProperties(std::uint64_t seed, std::string name) : seed_(seed), name_(std::move(name)) {
    }

    explicit WorldProperties(const NbtCompound& nbt) {
        seed_ = static_cast<std::uint64_t>(nbt.getLong("RandomSeed"));
        spawnX_ = nbt.getInt("SpawnX");
        spawnY_ = nbt.getInt("SpawnY");
        spawnZ_ = nbt.getInt("SpawnZ");
        time_ = static_cast<std::uint64_t>(nbt.getLong("Time"));
        lastPlayed_ = static_cast<std::uint64_t>(nbt.getLong("LastPlayed"));
        sizeOnDisk_ = static_cast<std::uint64_t>(nbt.getLong("SizeOnDisk"));
        name_ = nbt.getString("LevelName");
        version_ = nbt.getInt("version");
        rainTime_ = nbt.getInt("rainTime");
        raining_ = nbt.getBoolean("raining");
        thunderTime_ = nbt.getInt("thunderTime");
        thundering_ = nbt.getBoolean("thundering");
        if (nbt.contains("ModOptions")) {
            const NbtCompound options = nbt.getCompound("ModOptions");
            for (const auto& [key, value] : options.storage().asCompound()) {
                if (value.type() == Nbt::Type::String) {
                    modOptions_[key] = value.asString();
                }
            }
        }
        if (nbt.contains("Player")) {
            playerNbt_ = nbt.getCompound("Player");
            dimensionId_ = playerNbt_->getInt("Dimension");
        }
    }

    [[nodiscard]] NbtCompound asNbt() const {
        return asNbt(getPlayerNbt());
    }

    [[nodiscard]] NbtCompound asNbt(const NbtCompound* playerNbt) const {
        NbtCompound nbt;
        updateNbt(nbt, playerNbt);
        return nbt;
    }

    [[nodiscard]] NbtCompound asNbt(const std::vector<entity::player::PlayerEntity*>& players) const;

    [[nodiscard]] std::uint64_t getSeed() const noexcept {
        return seed_;
    }

    [[nodiscard]] int getSpawnX() const noexcept {
        return spawnX_;
    }

    [[nodiscard]] int getSpawnY() const noexcept {
        return spawnY_;
    }

    [[nodiscard]] int getSpawnZ() const noexcept {
        return spawnZ_;
    }

    [[nodiscard]] std::uint64_t getTime() const noexcept {
        return time_;
    }

    [[nodiscard]] std::uint64_t getLastPlayed() const noexcept {
        return lastPlayed_;
    }

    [[nodiscard]] std::uint64_t getSizeOnDisk() const noexcept {
        return sizeOnDisk_;
    }

    [[nodiscard]] int getDimensionId() const noexcept {
        return dimensionId_;
    }

    [[nodiscard]] const std::string& getName() const noexcept {
        return name_;
    }

    [[nodiscard]] int getVersion() const noexcept {
        return version_;
    }

    [[nodiscard]] bool getThundering() const noexcept {
        return thundering_;
    }

    [[nodiscard]] int getThunderTime() const noexcept {
        return thunderTime_;
    }

    [[nodiscard]] bool getRaining() const noexcept {
        return raining_;
    }

    [[nodiscard]] int getRainTime() const noexcept {
        return rainTime_;
    }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& getModOptions() const noexcept {
        return modOptions_;
    }

    void setSpawn(int x, int y, int z) noexcept {
        spawnX_ = x;
        spawnY_ = y;
        spawnZ_ = z;
    }

    void setSeed(std::uint64_t seed) noexcept {
        seed_ = seed;
    }

    void setTime(std::uint64_t time) noexcept {
        time_ = time;
    }

    void setSizeOnDisk(std::uint64_t size) noexcept {
        sizeOnDisk_ = size;
    }

    void setName(std::string name) {
        name_ = std::move(name);
    }

    void setVersion(int version) noexcept {
        version_ = version;
    }

    void setThundering(bool value) noexcept {
        thundering_ = value;
    }

    void setThunderTime(int time) noexcept {
        thunderTime_ = time;
    }

    void setRaining(bool value) noexcept {
        raining_ = value;
    }

    void setRainTime(int time) noexcept {
        rainTime_ = time;
    }

    void setModOptions(std::unordered_map<std::string, std::string> options) {
        modOptions_ = std::move(options);
    }

    void setPlayerNbt(NbtCompound playerNbt) {
        playerNbt_ = std::move(playerNbt);
        dimensionId_ = playerNbt_->getInt("Dimension");
    }

    void clearPlayerNbt() noexcept {
        playerNbt_.reset();
    }

    [[nodiscard]] const NbtCompound* getPlayerNbt() const noexcept {
        return playerNbt_.has_value() ? &*playerNbt_ : nullptr;
    }

    [[nodiscard]] std::uint64_t touchLastPlayed() noexcept {
        lastPlayed_ = nowMillis();
        return lastPlayed_;
    }

    [[nodiscard]] std::uint64_t setLastPlayed() noexcept {
        return touchLastPlayed();
    }

   private:
    static std::uint64_t nowMillis() {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    }

    void updateNbt(NbtCompound& nbt, const NbtCompound* playerNbt) const {
        static_cast<void>(const_cast<WorldProperties*>(this)->touchLastPlayed());
        nbt.putLong("RandomSeed", static_cast<std::int64_t>(seed_));
        nbt.putInt("SpawnX", spawnX_);
        nbt.putInt("SpawnY", spawnY_);
        nbt.putInt("SpawnZ", spawnZ_);
        nbt.putLong("Time", static_cast<std::int64_t>(time_));
        nbt.putLong("SizeOnDisk", static_cast<std::int64_t>(sizeOnDisk_));
        nbt.putLong("LastPlayed", static_cast<std::int64_t>(lastPlayed_));
        nbt.putString("LevelName", name_);
        nbt.putInt("version", version_);
        nbt.putInt("rainTime", rainTime_);
        nbt.putBoolean("raining", raining_);
        nbt.putInt("thunderTime", thunderTime_);
        nbt.putBoolean("thundering", thundering_);
        if (!modOptions_.empty()) {
            NbtCompound options;
            for (const auto& [key, value] : modOptions_) {
                options.putString(key, value);
            }
            nbt.put("ModOptions", options);
        }
        if (playerNbt != nullptr) {
            nbt.put("Player", *playerNbt);
        }
    }

    std::uint64_t seed_ = 0;
    int spawnX_ = 0;
    int spawnY_ = 0;
    int spawnZ_ = 0;
    std::uint64_t time_ = 0;
    std::uint64_t lastPlayed_ = 0;
    std::uint64_t sizeOnDisk_ = 0;
    std::optional<NbtCompound> playerNbt_;
    int dimensionId_ = 0;
    std::string name_;
    int version_ = 0;
    bool raining_ = false;
    int rainTime_ = 0;
    bool thundering_ = false;
    int thunderTime_ = 0;
    std::unordered_map<std::string, std::string> modOptions_;
};
}  // namespace net::minecraft
