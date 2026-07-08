#include "net/minecraft/world/NaturalSpawner.hpp"

#include <array>
#include <cmath>
#include <optional>
#include <unordered_set>
#include <vector>

#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/Monster.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"

namespace net::minecraft {
namespace {
struct SpawnRules {
    int biomeGroup = 0;
    int capacity = 0;
    bool waterSpawn = false;
    bool peaceful = false;
};

constexpr SpawnRules kSpawnRules[] = {
    {0, 70, false, false},
    {1, 15, false, true},
    {2, 5, true, true},
};

[[nodiscard]] const block::material::Material& spawnMaterial(const SpawnRules& rules) {
    return rules.waterSpawn ? block::material::Material::WATER : block::material::Material::AIR;
}

[[nodiscard]] bool isValidSpawnPos(const SpawnRules& rules, World* world, int x, int y, int z) {
    if (world == nullptr) {
        return false;
    }
    if (rules.waterSpawn) {
        return world->getMaterial(x, y, z).isFluid() && !world->shouldSuffocate(x, y + 1, z);
    }
    return world->shouldSuffocate(x, y - 1, z) && !world->shouldSuffocate(x, y, z) &&
           !world->getMaterial(x, y, z).isFluid() && !world->shouldSuffocate(x, y + 1, z);
}

[[nodiscard]] const EntitySpawnGroup* pickSpawnEntry(const std::vector<EntitySpawnGroup>& entries, JavaRandom& random) {
    int totalWeight = 0;
    for (const EntitySpawnGroup& entry : entries) {
        totalWeight += entry.amount;
    }
    if (totalWeight <= 0) {
        return nullptr;
    }
    int pick = random.nextInt(totalWeight);
    for (const EntitySpawnGroup& entry : entries) {
        pick -= entry.amount;
        if (pick < 0) {
            return &entry;
        }
    }
    return nullptr;
}

void postSpawnEntity(LivingEntity* entity, World* world, float x, float y, float z) {
    if (entity == nullptr || world == nullptr) {
        return;
    }
    if (auto* spider = dynamic_cast<entity::mob::SpiderEntity*>(entity)) {
        (void) spider;
        if (world->random().nextInt(100) == 0) {
            LivingEntity* skeleton = world->spawnMob("Skeleton", x, y, z, entity->yaw, 0.0f);
            if (skeleton == nullptr) {
                return;
            }
            skeleton->setVehicle(entity);
        }
    }
}

[[nodiscard]] std::array<int, 3> countSpawnBuckets(const World& world) {
    std::array<int, 3> counts{};
    for (Entity* entity : world.entities()) {
        if (entity == nullptr || entity->dead) {
            continue;
        }
        if (dynamic_cast<entity::Monster*>(entity) != nullptr) {
            ++counts[0];
        } else if (dynamic_cast<entity::passive::AnimalEntity*>(entity) != nullptr) {
            ++counts[1];
        } else if (dynamic_cast<entity::WaterCreatureEntity*>(entity) != nullptr) {
            ++counts[2];
        }
    }
    return counts;
}

[[nodiscard]] bool shouldSkipSpawnBucket(const SpawnRules& rules,
                                         bool spawnAnimals,
                                         bool spawnMonsters,
                                         std::size_t chunkCount,
                                         const std::array<int, 3>& bucketCounts) {
    if (rules.peaceful && !spawnMonsters) {
        return true;
    }
    if (!rules.peaceful && !spawnAnimals) {
        return true;
    }
    const int population = bucketCounts[static_cast<std::size_t>(rules.biomeGroup)];
    const int capacity = rules.capacity * static_cast<int>(chunkCount) / 256;
    return population > capacity;
}
}  // namespace

Vec3i NaturalSpawner::getRandomPosInChunk(World* world, int chunkX, int chunkZ) {
    if (world == nullptr) {
        return Vec3i{chunkX, 64, chunkZ};
    }
    return Vec3i{
        chunkX + world->random().nextInt(16), world->random().nextInt(128), chunkZ + world->random().nextInt(16)};
}

int NaturalSpawner::tick(World* world, bool spawnAnimals, bool spawnMonsters) {
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
    const std::array<int, 3> bucketCounts = countSpawnBuckets(*world);
    for (int groupIndex = 0; groupIndex < 3; ++groupIndex) {
        const SpawnRules& rules = kSpawnRules[groupIndex];
        const block::material::Material& requiredMaterial = spawnMaterial(rules);
        if (shouldSkipSpawnBucket(rules, spawnAnimals, spawnMonsters, mobSpawningChunks_.size(), bucketCounts)) {
            continue;
        }
        for (const ChunkPos& chunkPos : mobSpawningChunks_) {
            const Biome& biomeDef = world->getBiome(chunkPos.x * 16, chunkPos.z * 16);
            const EntitySpawnGroup* chosen =
                pickSpawnEntry(biomeDef.getSpawnableEntities(rules.biomeGroup), world->random());
            if (chosen == nullptr) {
                continue;
            }
            const Vec3i blockPos = getRandomPosInChunk(world, chunkPos.x * 16, chunkPos.z * 16);
            const int bx = blockPos.x;
            const int by = blockPos.y;
            const int bz = blockPos.z;
            if (world->shouldSuffocate(bx, by, bz) || &world->getMaterial(bx, by, bz) != &requiredMaterial) {
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
                    if (!isValidSpawnPos(rules, world, px, py, pz)) {
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
                    LivingEntity* living = world->spawnMob(chosen->entityType, [&](LivingEntity& mob) {
                        limitPerChunk = mob.getLimitPerChunk();
                        mob.setPositionAndAnglesKeepPrevAngles(fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
                        return mob.canSpawn();
                    });
                    if (living == nullptr) {
                        continue;
                    }
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

bool NaturalSpawner::spawnMonstersAndWakePlayers(World* world, std::vector<PlayerEntity*>& players) {
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
            while (!isValidSpawnPos(kSpawnRules[0], world, x, groundY, z) && groundY < y + 16 && groundY < 128) {
                ++groundY;
            }
            if (groundY >= y + 16 || groundY >= 128) {
                groundY = y;
                continue;
            }
            const float fx = static_cast<float>(x) + 0.5f;
            const float fy = static_cast<float>(groundY);
            const float fz = static_cast<float>(z) + 0.5f;
            std::optional<Vec3i> wakePos;
            LivingEntity* living = world->spawnMob(monsterId, [&](LivingEntity& mob) {
                mob.setPositionAndAnglesKeepPrevAngles(fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
                if (!mob.canSpawn()) {
                    return false;
                }
                entity::ai::pathing::Path path = pathNodeNavigator.findPath(&mob, playerEntity, 32.0f);
                if (path.length <= 1) {
                    return false;
                }
                const entity::ai::pathing::PathNode* pathEnd = path.getEnd();
                if (pathEnd == nullptr) {
                    return false;
                }
                if (std::abs(static_cast<double>(pathEnd->x) - playerEntity->x) >= 1.5 ||
                    std::abs(static_cast<double>(pathEnd->z) - playerEntity->z) >= 1.5 ||
                    std::abs(static_cast<double>(pathEnd->y) - playerEntity->y) >= 1.5) {
                    return false;
                }
                wakePos = block::BedBlock::findWakeUpPosition(world,
                                                              MathHelper::floor(playerEntity->x),
                                                              MathHelper::floor(playerEntity->y),
                                                              MathHelper::floor(playerEntity->z),
                                                              1);
                if (!wakePos.has_value()) {
                    wakePos = Vec3i{x, groundY + 1, z};
                }
                mob.setPositionAndAnglesKeepPrevAngles(static_cast<float>(wakePos->x) + 0.5f,
                                                       static_cast<float>(wakePos->y),
                                                       static_cast<float>(wakePos->z) + 0.5f,
                                                       0.0f,
                                                       0.0f);
                return true;
            });
            if (living == nullptr || !wakePos.has_value()) {
                continue;
            }
            postSpawnEntity(living,
                            world,
                            static_cast<float>(wakePos->x) + 0.5f,
                            static_cast<float>(wakePos->y),
                            static_cast<float>(wakePos->z) + 0.5f);
            playerEntity->wakeUp(true, false, false);
            living->makeSound();
            spawnedAny = true;
            spawnedNearPlayer = true;
        }
    }
    return spawnedAny;
}
}  // namespace net::minecraft
