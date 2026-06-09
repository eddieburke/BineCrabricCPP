#include "net/minecraft/world/NaturalSpawner.hpp"

#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/SpawnGroup.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/biome/EntitySpawnGroup.hpp"

#include <cmath>
#include <memory>
#include <unordered_set>
#include <vector>

namespace net::minecraft {

namespace {

[[nodiscard]] bool isValidSpawnPos(const entity::SpawnGroup& spawnGroup, World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return false;
    }
    if (spawnGroup.isWaterSpawn()) {
        return world->getMaterial(x, y, z).isFluid() && !world->shouldSuffocate(x, y + 1, z);
    }
    return world->shouldSuffocate(x, y - 1, z) && !world->shouldSuffocate(x, y, z)
        && !world->getMaterial(x, y, z).isFluid() && !world->shouldSuffocate(x, y + 1, z);
}

void postSpawnEntity(LivingEntity* entity, World* world, float x, float y, float z)
{
    if (entity == nullptr || world == nullptr) {
        return;
    }
    if (auto* spider = dynamic_cast<entity::mob::SpiderEntity*>(entity)) {
        (void)spider;
        if (world->random().nextInt(100) == 0) {
            auto skeleton = std::make_unique<entity::mob::SkeletonEntity>(world);
            skeleton->setPositionAndAnglesKeepPrevAngles(x, y, z, entity->yaw, 0.0f);
            Entity* skeletonRaw = skeleton.get();
            world->spawnEntity(skeleton.release());
            skeletonRaw->setVehicle(entity);
        }
    } else if (auto* sheep = dynamic_cast<entity::passive::SheepEntity*>(entity)) {
        sheep->setColor(entity::passive::SheepEntity::generateDefaultColor(world->random()));
    }
}

[[nodiscard]] bool shouldSkipSpawnGroup(const entity::SpawnGroup& spawnGroup, World* world,
    bool spawnAnimals, bool spawnMonsters, std::size_t chunkCount)
{
    if (spawnGroup.isPeaceful() && !spawnMonsters) {
        return true;
    }
    if (!spawnGroup.isPeaceful() && !spawnAnimals) {
        return true;
    }
    const int population = world->countSpawnGroup(spawnGroup.kind);
    const int capacity = spawnGroup.getCapacity() * static_cast<int>(chunkCount) / 256;
    return population > capacity;
}

} // namespace

Vec3i NaturalSpawner::getRandomPosInChunk(World* world, int chunkX, int chunkZ)
{
    if (world == nullptr) {
        return Vec3i{chunkX, 64, chunkZ};
    }
    return Vec3i{
        chunkX + world->random().nextInt(16),
        world->random().nextInt(128),
        chunkZ + world->random().nextInt(16)};
}

int NaturalSpawner::tick(World* world, bool spawnAnimals, bool spawnMonsters)
{
    if (world == nullptr || (!spawnAnimals && !spawnMonsters)) {
        return 0;
    }

    mobSpawningChunks_.clear();
    for (PlayerEntity* player : world->players) {
        if (player == nullptr) {
            continue;
        }
        const int playerChunkX = MathHelper::floor(player->x / 16.0);
        const int playerChunkZ = MathHelper::floor(player->z / 16.0);
        constexpr int radius = 8;
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                mobSpawningChunks_.insert(ChunkPos{playerChunkX + dx, playerChunkZ + dz});
            }
        }
    }

    if (mobSpawningChunks_.empty()) {
        return 0;
    }

    int spawned = 0;
    const Vec3i spawnPos = world->getSpawnPos();
    const entity::SpawnGroup spawnGroups[] = {
        entity::kSpawnGroupMonster,
        entity::kSpawnGroupCreature,
        entity::kSpawnGroupWaterCreature,
    };

    for (const entity::SpawnGroup& spawnGroup : spawnGroups) {
        if (shouldSkipSpawnGroup(spawnGroup, world, spawnAnimals, spawnMonsters, mobSpawningChunks_.size())) {
            continue;
        }

        const int groupIndex = static_cast<int>(spawnGroup.kind);
        for (const ChunkPos& chunkPos : mobSpawningChunks_) {
            if (world->getBiomeSource() == nullptr) {
                continue;
            }
            const BiomeInfo biome = world->getBiomeSource()->getBiome(chunkPos.x * 16, chunkPos.z * 16);
            const BiomeDefinition& biomeDef = Biomes::byInfo(biome);
            const std::vector<EntitySpawnGroup>& list = biomeDef.getSpawnableEntities(groupIndex);
            if (list.empty()) {
                continue;
            }

            int totalWeight = 0;
            for (const EntitySpawnGroup& entry : list) {
                totalWeight += entry.amount;
            }
            if (totalWeight <= 0) {
                continue;
            }

            int pick = world->random().nextInt(totalWeight);
            const EntitySpawnGroup* chosen = &list.front();
            for (const EntitySpawnGroup& entry : list) {
                pick -= entry.amount;
                if (pick < 0) {
                    chosen = &entry;
                    break;
                }
            }

            const Vec3i blockPos = getRandomPosInChunk(world, chunkPos.x * 16, chunkPos.z * 16);
            const int bx = blockPos.x;
            const int by = blockPos.y;
            const int bz = blockPos.z;
            if (world->shouldSuffocate(bx, by, bz) || &world->getMaterial(bx, by, bz) != &spawnGroup.getSpawnMaterial()) {
                continue;
            }

            int spawnedInChunk = 0;
            int limitPerChunk = 4;
            for (int attempt = 0; attempt < 3; ++attempt) {
                int px = bx;
                int py = by;
                int pz = bz;
                constexpr int spread = 6;
                for (int jitter = 0; jitter < 4; ++jitter) {
                    px += world->random().nextInt(spread) - world->random().nextInt(spread);
                    py += world->random().nextInt(1) - world->random().nextInt(1);
                    pz += world->random().nextInt(spread) - world->random().nextInt(spread);
                    if (!isValidSpawnPos(spawnGroup, world, px, py, pz)) {
                        continue;
                    }

                    const float fx = static_cast<float>(px) + 0.5f;
                    const float fy = static_cast<float>(py);
                    const float fz = static_cast<float>(pz) + 0.5f;
                    if (world->getClosestPlayer(fx, fy, fz, 24.0) != nullptr) {
                        continue;
                    }

                    const double dx = static_cast<double>(fx - static_cast<float>(spawnPos.x));
                    const double dy = static_cast<double>(fy - static_cast<float>(spawnPos.y));
                    const double dz = static_cast<double>(fz - static_cast<float>(spawnPos.z));
                    if (dx * dx + dy * dy + dz * dz < 576.0) {
                        continue;
                    }

                    std::unique_ptr<Entity> entity = EntityRegistry::create(chosen->entityType, world);
                    if (entity == nullptr) {
                        return spawned;
                    }
                    auto* living = dynamic_cast<LivingEntity*>(entity.get());
                    if (living == nullptr) {
                        return spawned;
                    }
                    limitPerChunk = living->getLimitPerChunk();
                    living->setPositionAndAnglesKeepPrevAngles(
                        fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
                    if (!living->canSpawn()) {
                        continue;
                    }
                    world->spawnEntity(entity.release());
                    postSpawnEntity(living, world, fx, fy, fz);
                    ++spawnedInChunk;
                    ++spawned;
                    if (spawnedInChunk >= limitPerChunk) {
                        break;
                    }
                }
                if (spawnedInChunk >= limitPerChunk) {
                    break;
                }
            }
        }
    }

    return spawned;
}

bool NaturalSpawner::spawnMonstersAndWakePlayers(World* world, std::vector<PlayerEntity*>& players)
{
    if (world == nullptr || players.empty()) {
        return false;
    }

    bool spawnedAny = false;
    entity::ai::pathing::PathNodeNavigator pathNodeNavigator(world);
    for (PlayerEntity* playerEntity : players) {
        if (playerEntity == nullptr) {
            continue;
        }
        bool spawnedNearPlayer = false;
        for (int attempt = 0; attempt < 20 && !spawnedNearPlayer; ++attempt) {
            int x = MathHelper::floor(playerEntity->x) + world->random().nextInt(32) - world->random().nextInt(32);
            int z = MathHelper::floor(playerEntity->z) + world->random().nextInt(32) - world->random().nextInt(32);
            int y = MathHelper::floor(playerEntity->y) + world->random().nextInt(16) - world->random().nextInt(16);
            if (y < 1) {
                y = 1;
            } else if (y > 128) {
                y = 128;
            }

            const int monsterIndex = world->random().nextInt(static_cast<int>(MONSTER_TYPE.size()));
            const std::string& monsterId = MONSTER_TYPE[static_cast<std::size_t>(monsterIndex)];

            int groundY = y;
            for (; groundY > 2 && !world->shouldSuffocate(x, groundY - 1, z); --groundY) {
            }
            while (!isValidSpawnPos(entity::kSpawnGroupMonster, world, x, groundY, z) && groundY < y + 16 && groundY < 128) {
                ++groundY;
            }
            if (groundY >= y + 16 || groundY >= 128) {
                groundY = y;
                continue;
            }

            const float fx = static_cast<float>(x) + 0.5f;
            const float fy = static_cast<float>(groundY);
            const float fz = static_cast<float>(z) + 0.5f;

            std::unique_ptr<Entity> entity = EntityRegistry::create(monsterId, world);
            if (entity == nullptr) {
                return spawnedAny;
            }
            auto* living = dynamic_cast<LivingEntity*>(entity.get());
            if (living == nullptr) {
                return spawnedAny;
            }
            living->setPositionAndAnglesKeepPrevAngles(fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
            if (!living->canSpawn()) {
                continue;
            }

            entity::ai::pathing::Path path = pathNodeNavigator.findPath(living, playerEntity, 32.0f);
            if (path.length <= 1) {
                continue;
            }
            const entity::ai::pathing::PathNode* pathEnd = path.getEnd();
            if (pathEnd == nullptr) {
                continue;
            }
            if (std::abs(static_cast<double>(pathEnd->x) - playerEntity->x) >= 1.5
                || std::abs(static_cast<double>(pathEnd->z) - playerEntity->z) >= 1.5
                || std::abs(static_cast<double>(pathEnd->y) - playerEntity->y) >= 1.5) {
                continue;
            }

            std::optional<Vec3i> wakePos = block::BedBlock::findWakeUpPosition(
                world, MathHelper::floor(playerEntity->x), MathHelper::floor(playerEntity->y),
                MathHelper::floor(playerEntity->z), 1);
            if (!wakePos.has_value()) {
                wakePos = Vec3i{x, groundY + 1, z};
            }

            living->setPositionAndAnglesKeepPrevAngles(
                static_cast<float>(wakePos->x) + 0.5f, static_cast<float>(wakePos->y),
                static_cast<float>(wakePos->z) + 0.5f, 0.0f, 0.0f);
            world->spawnEntity(entity.release());
            postSpawnEntity(living, world, static_cast<float>(wakePos->x) + 0.5f, static_cast<float>(wakePos->y),
                static_cast<float>(wakePos->z) + 0.5f);
            playerEntity->wakeUp(true, false, false);
            living->makeSound();
            spawnedAny = true;
            spawnedNearPlayer = true;
        }
    }
    return spawnedAny;
}

} // namespace net::minecraft
