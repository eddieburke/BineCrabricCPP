#pragma once

#include "net/minecraft/block/entity/BlockEntity.hpp"

#include <string>

namespace net::minecraft::block::entity {

class MobSpawnerBlockEntity : public BlockEntity {
public:
    [[nodiscard]] const std::string& getSpawnedEntityId() const noexcept
    {
        return spawnedEntityId_;
    }

    void setSpawnedEntityId(std::string spawnedEntityId);
    [[nodiscard]] bool isPlayerInRange() const;
    void tick() override;
    void readNbt(const NbtCompound& nbt) override;
    void writeNbt(NbtCompound& nbt) const override;
    [[nodiscard]] std::string id() const override
    {
        return "MobSpawner";
    }

    int spawnDelay = 20;
    double rotation = 0.0;
    double lastRotation = 0.0;

private:
    void resetDelay();

    std::string spawnedEntityId_ = "Pig";
};

} // namespace net::minecraft::block::entity
