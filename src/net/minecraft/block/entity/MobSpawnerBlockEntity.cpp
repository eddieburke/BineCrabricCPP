#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"

#include <typeinfo>

#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block::entity {
void MobSpawnerBlockEntity::setSpawnedEntityId(std::string spawnedEntityId) {
    spawnedEntityId_ = std::move(spawnedEntityId);
}

bool MobSpawnerBlockEntity::isPlayerInRange() const {
    if (world == nullptr) {
        return false;
    }
    return world->getClosestPlayer(
               static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5, 16.0) !=
           nullptr;
}

void MobSpawnerBlockEntity::resetDelay() {
    if (world == nullptr) {
        return;
    }
    spawnDelay = 200 + world->random().nextInt(600);
}

void MobSpawnerBlockEntity::tick() {
    lastRotation = rotation;
    if (!isPlayerInRange()) {
        return;
    }
    if (world != nullptr) {
        const double px = static_cast<double>(x) + world->random().nextFloat();
        const double py = static_cast<double>(y) + world->random().nextFloat();
        const double pz = static_cast<double>(z) + world->random().nextFloat();
        world->addParticle("smoke", px, py, pz, 0.0, 0.0, 0.0);
        world->addParticle("flame", px, py, pz, 0.0, 0.0, 0.0);
    }
    rotation += 1000.0 / (static_cast<double>(spawnDelay) + 200.0);
    while (rotation > 360.0) {
        rotation -= 360.0;
        lastRotation -= 360.0;
    }
    if (world != nullptr && !world->isRemote()) {
        if (spawnDelay == -1) {
            resetDelay();
        }
        if (spawnDelay > 0) {
            --spawnDelay;
            return;
        }
        constexpr int spawnAttempts = 4;
        for (int attempt = 0; attempt < spawnAttempts; ++attempt) {
            bool tooManyNearby = false;
            LivingEntity* living = world->spawnMob(spawnedEntityId_, [&](LivingEntity& mob) {
                const Box spawnBox{static_cast<double>(x),
                                   static_cast<double>(y),
                                   static_cast<double>(z),
                                   static_cast<double>(x + 1),
                                   static_cast<double>(y + 1),
                                   static_cast<double>(z + 1)};
                const Box expandedBox{spawnBox.minX - 8.0,
                                      spawnBox.minY - 4.0,
                                      spawnBox.minZ - 8.0,
                                      spawnBox.maxX + 8.0,
                                      spawnBox.maxY + 4.0,
                                      spawnBox.maxZ + 8.0};
                int nearbyCount = 0;
                for (Entity* nearby : world->getEntities(nullptr, expandedBox)) {
                    if (nearby != nullptr && typeid(*nearby) == typeid(mob)) {
                        ++nearbyCount;
                    }
                }
                if (nearbyCount >= 6) {
                    tooManyNearby = true;
                    return false;
                }
                const double spawnX =
                    static_cast<double>(x) + (world->random().nextDouble() - world->random().nextDouble()) * 4.0;
                const double spawnY = static_cast<double>(y) + world->random().nextInt(3) - 1;
                const double spawnZ =
                    static_cast<double>(z) + (world->random().nextDouble() - world->random().nextDouble()) * 4.0;
                mob.setPositionAndAnglesKeepPrevAngles(
                    spawnX, spawnY, spawnZ, world->random().nextFloat() * 360.0f, 0.0f);
                return mob.canSpawn();
            });
            if (tooManyNearby) {
                resetDelay();
                return;
            }
            if (living == nullptr) {
                continue;
            }
            for (int particle = 0; particle < 20; ++particle) {
                const double particleX = static_cast<double>(x) + 0.5 + (world->random().nextFloat() - 0.5f) * 2.0f;
                const double particleY = static_cast<double>(y) + 0.5 + (world->random().nextFloat() - 0.5f) * 2.0f;
                const double particleZ = static_cast<double>(z) + 0.5 + (world->random().nextFloat() - 0.5f) * 2.0f;
                world->addParticle("smoke", particleX, particleY, particleZ, 0.0, 0.0, 0.0);
                world->addParticle("flame", particleX, particleY, particleZ, 0.0, 0.0, 0.0);
            }
            living->animateSpawn();
            resetDelay();
        }
    }
    BlockEntity::tick();
}

void MobSpawnerBlockEntity::readNbt(const NbtCompound& nbt) {
    BlockEntity::readNbt(nbt);
    spawnedEntityId_ = nbt.getString("EntityId");
    spawnDelay = nbt.getShort("Delay");
}

void MobSpawnerBlockEntity::writeNbt(NbtCompound& nbt) const {
    BlockEntity::writeNbt(nbt);
    nbt.putString("EntityId", spawnedEntityId_);
    nbt.putShort("Delay", static_cast<std::int16_t>(spawnDelay));
}
}  // namespace net::minecraft::block::entity
