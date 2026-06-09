#pragma once

#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block::material {
class Material;
}

namespace net::minecraft::entity {

enum class SpawnGroupKind {
    Monster,
    Creature,
    WaterCreature,
};

// Faithful port of net.minecraft.entity.SpawnGroup (beta 1.7.3).
struct SpawnGroup {
    SpawnGroupKind kind = SpawnGroupKind::Monster;
    int capacity = 70;
    bool waterSpawn = false;
    bool peaceful = false;

    [[nodiscard]] bool isPeaceful() const noexcept { return peaceful; }
    [[nodiscard]] int getCapacity() const noexcept { return capacity; }
    [[nodiscard]] bool isWaterSpawn() const noexcept { return waterSpawn; }

    [[nodiscard]] const block::material::Material& getSpawnMaterial() const noexcept
    {
        return waterSpawn ? block::material::Material::WATER : block::material::Material::AIR;
    }
};

inline constexpr SpawnGroup kSpawnGroupMonster{SpawnGroupKind::Monster, 70, false, false};
inline constexpr SpawnGroup kSpawnGroupCreature{SpawnGroupKind::Creature, 15, false, true};
inline constexpr SpawnGroup kSpawnGroupWaterCreature{SpawnGroupKind::WaterCreature, 5, true, true};

} // namespace net::minecraft::entity
