#include "net/minecraft/world/NaturalSpawner.hpp"
#include <array>
#include <cmath>
#include <optional>
#include <typeinfo>
#include <utility>
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
[[nodiscard]] bool playerWithinRange(const std::vector<Vec3d>& spots, float x, float y, float z) {
 for(const Vec3d& spot : spots) {
  const double dx = static_cast<double>(x) - spot.x;
  const double dy = static_cast<double>(y) - spot.y;
  const double dz = static_cast<double>(z) - spot.z;
  if(dx * dx + dy * dy + dz * dz < 576.0) {
   return true;
  }
 }
 return false;
}
[[nodiscard]] block::Block* blockOf(int id) {
 return id > 0 && id < block::Block::BLOCK_COUNT ? block::Block::BLOCKS[static_cast<std::size_t>(id)] : nullptr;
}
[[nodiscard]] bool solidFullCube(block::Block* block) {
 return block != nullptr && block->material.suffocates() && block->isFullCube();
}
constexpr SpawnRules kSpawnRules[] = {
    {0, 70, false, false},
    {1, 15, false, true},
    {2, 5, true, true},
};
[[nodiscard]] const SpawnListEntry* pickSpawnEntry(const std::vector<SpawnListEntry>& entries, JavaRandom& random) {
 int totalWeight = 0;
 for(const SpawnListEntry& entry : entries) {
  totalWeight += entry.itemWeight;
 }
 if(totalWeight <= 0) {
  return nullptr;
 }
 int pick = random.nextInt(totalWeight);
 for(const SpawnListEntry& entry : entries) {
  pick -= entry.itemWeight;
  if(pick < 0) {
   return &entry;
  }
 }
 return nullptr;
}
void postSpawnEntity(LivingEntity* entity, World* world, float x, float y, float z) {
 if(entity == nullptr || world == nullptr) {
  return;
 }
 if(dynamic_cast<entity::mob::SpiderEntity*>(entity) != nullptr) {
  if(world->random().nextInt(100) == 0) {
   LivingEntity* skeleton = world->spawnMob("Skeleton", x, y, z, entity->yaw, 0.0f);
   if(skeleton == nullptr) {
    return;
   }
   skeleton->setVehicle(entity);
  }
 }
}
[[nodiscard]] int creatureTypeOf(Entity* entity) {
 // The three creature bases are cross-cast siblings, so every dynamic_cast here is a
 // hierarchy walk and a miss is the slow path - which is every item, arrow and boat in
 // the world, on every spawn tick. The answer depends only on the dynamic type, so pay
 // for it once per concrete class.
 static std::vector<std::pair<const std::type_info*, int>> classified;
 const std::type_info* type = &typeid(*entity);
 for(const std::pair<const std::type_info*, int>& entry : classified) {
  if(entry.first == type) {
   return entry.second;
  }
 }
 const int creatureType = dynamic_cast<entity::Monster*>(entity) != nullptr                 ? 0
                          : dynamic_cast<entity::passive::AnimalEntity*>(entity) != nullptr ? 1
                          : dynamic_cast<entity::WaterCreatureEntity*>(entity) != nullptr   ? 2
                                                                                            : -1;
 classified.emplace_back(type, creatureType);
 return creatureType;
}
[[nodiscard]] std::array<int, 3> countLiveMobsByCreatureType(const World& world) {
 std::array<int, 3> counts{};
 for(Entity* entity : world.entities()) {
  if(entity == nullptr || entity->dead) {
   continue;
  }
  if(const int creatureType = creatureTypeOf(entity); creatureType >= 0) {
   ++counts[static_cast<std::size_t>(creatureType)];
  }
 }
 return counts;
}
[[nodiscard]] bool shouldSkipCreatureType(const SpawnRules& rules,
                                          bool spawnHostile,
                                          bool spawnPeaceful,
                                          std::size_t chunkCount,
                                          const std::array<int, 3>& liveCounts) {
 if(rules.peaceful && !spawnPeaceful) {
  return true;
 }
 if(!rules.peaceful && !spawnHostile) {
  return true;
 }
 const int liveCount = liveCounts[static_cast<std::size_t>(rules.creatureType)];
 const int capacity = rules.capacity * static_cast<int>(chunkCount) / 256;
 return liveCount > capacity;
}
} // namespace
// see third_party/mcp/net/minecraft/src/SpawnerAnimals.java:175
bool isValidSpawnPos(const SpawnRules& rules, World* world, int x, int y, int z) {
 if(world == nullptr) {
  return false;
 }
 if(rules.waterSpawn) {
  block::Block* const at = blockOf(world->getBlockId(x, y, z));
  if(at == nullptr || !at->material.isFluid()) {
   return false;
  }
  return !solidFullCube(blockOf(world->getBlockId(x, y + 1, z)));
 }
 if(!solidFullCube(blockOf(world->getBlockId(x, y - 1, z)))) {
  return false;
 }
 block::Block* const at = blockOf(world->getBlockId(x, y, z));
 if(at != nullptr && (solidFullCube(at) || at->material.isFluid())) {
  return false;
 }
 return !solidFullCube(blockOf(world->getBlockId(x, y + 1, z)));
}
Vec3i NaturalSpawner::getRandomPosInChunk(World* world, int chunkX, int chunkZ) {
 if(world == nullptr) {
  return Vec3i{chunkX, 64, chunkZ};
 }
 return Vec3i{
     chunkX + world->random().nextInt(16), world->random().nextInt(128), chunkZ + world->random().nextInt(16)};
}
int NaturalSpawner::tick(World* world, bool spawnHostile, bool spawnPeaceful) {
 if(world == nullptr || (!spawnHostile && !spawnPeaceful)) {
  return 0;
 }
 mobSpawningChunks_.clear();
 std::vector<Vec3d> playerSpots;
 playerSpots.reserve(world->players.size());
 for(PlayerEntity* player : world->players) {
  if(player == nullptr) {
   continue;
  }
  playerSpots.push_back(Vec3d{player->x, player->y, player->z});
  const int playerChunkX = MathHelper::floor(player->x / 16.0);
  const int playerChunkZ = MathHelper::floor(player->z / 16.0);
  constexpr int radius = 8;
  for(int dx = -radius; dx <= radius; ++dx) {
   for(int dz = -radius; dz <= radius; ++dz) {
    mobSpawningChunks_.insert(ChunkPos{playerChunkX + dx, playerChunkZ + dz});
   }
  }
 }
 if(mobSpawningChunks_.empty()) {
  return 0;
 }
 int spawned = 0;
 const Vec3i spawnPos = world->getSpawnPos();
 const std::array<int, 3> liveCounts = countLiveMobsByCreatureType(*world);
 struct ChunkBiome {
  ChunkPos pos;
  const Biome* biome;
 };
 std::vector<ChunkBiome> chunkBiomes;
 chunkBiomes.reserve(mobSpawningChunks_.size());
 for(const ChunkPos& chunkPos : mobSpawningChunks_) {
  chunkBiomes.push_back(ChunkBiome{chunkPos, &world->getBiome(chunkPos.x * 16, chunkPos.z * 16)});
 }
 for(int creatureType = 0; creatureType < 3; ++creatureType) {
  const SpawnRules& rules = kSpawnRules[creatureType];
  const block::material::Material& requiredMaterial =
      rules.waterSpawn ? block::material::Material::WATER : block::material::Material::AIR;
  if(shouldSkipCreatureType(rules, spawnHostile, spawnPeaceful, mobSpawningChunks_.size(), liveCounts)) {
   continue;
  }
  for(const ChunkBiome& chunk : chunkBiomes) {
   const ChunkPos& chunkPos = chunk.pos;
   const Biome& biomeDef = *chunk.biome;
   const SpawnListEntry* chosen =
       pickSpawnEntry(biomeDef.getSpawnableEntities(rules.creatureType), world->random());
   if(chosen == nullptr) {
    continue;
   }
   const Vec3i blockPos = getRandomPosInChunk(world, chunkPos.x * 16, chunkPos.z * 16);
   const int bx = blockPos.x;
   const int by = blockPos.y;
   const int bz = blockPos.z;
   block::Block* const preBlock = blockOf(world->getBlockId(bx, by, bz));
   block::material::Material& preMaterial =
       preBlock != nullptr ? preBlock->material : block::material::Material::AIR;
   if(solidFullCube(preBlock) || &preMaterial != &requiredMaterial) {
    continue;
   }
   int spawnedInChunk = 0;
   int limitPerChunk = 4;
   for(int attempt = 0; attempt < 3; ++attempt) {
    int px = bx;
    int py = by;
    int pz = bz;
    constexpr int spread = 6;
    for(int jitter = 0; jitter < 4; ++jitter) {
     px += world->random().nextInt(spread) - world->random().nextInt(spread);
     py += world->random().nextInt(1) - world->random().nextInt(1);
     pz += world->random().nextInt(spread) - world->random().nextInt(spread);
     if(!isValidSpawnPos(rules, world, px, py, pz)) {
      continue;
     }
     const float fx = static_cast<float>(px) + 0.5f;
     const float fy = static_cast<float>(py);
     const float fz = static_cast<float>(pz) + 0.5f;
     if(playerWithinRange(playerSpots, fx, fy, fz)) {
      continue;
     }
     const double dx = static_cast<double>(fx - static_cast<float>(spawnPos.x));
     const double dy = static_cast<double>(fy - static_cast<float>(spawnPos.y));
     const double dz = static_cast<double>(fz - static_cast<float>(spawnPos.z));
     if(dx * dx + dy * dy + dz * dz < 576.0) {
      continue;
     }
     entity::LivingEntity* living = world->spawnMob(chosen->entityType, [&](entity::LivingEntity& mob) {
      limitPerChunk = mob.getLimitPerChunk();
      mob.setPositionAndAnglesKeepPrevAngles(fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
      return mob.canSpawn() && (rules.creatureType != 2 || world->canSpawnEntity(mob.boundingBox));
     });
     if(living == nullptr) {
      continue;
     }
     postSpawnEntity(living, world, fx, fy, fz);
     ++spawnedInChunk;
     ++spawned;
    }
    if(spawnedInChunk >= limitPerChunk) {
     break;
    }
   }
  }
 }
 return spawned;
}
bool NaturalSpawner::spawnMonstersAndWakePlayers(World* world, std::vector<PlayerEntity*>& players) {
 if(world == nullptr || players.empty()) {
  return false;
 }
 bool spawnedAny = false;
 entity::ai::pathing::PathNodeNavigator pathNodeNavigator(world);
 for(PlayerEntity* playerEntity : players) {
  if(playerEntity == nullptr) {
   continue;
  }
  bool spawnedNearPlayer = false;
  for(int attempt = 0; attempt < 20 && !spawnedNearPlayer; ++attempt) {
   int x = MathHelper::floor(playerEntity->x) + world->random().nextInt(32) - world->random().nextInt(32);
   int z = MathHelper::floor(playerEntity->z) + world->random().nextInt(32) - world->random().nextInt(32);
   int y = MathHelper::floor(playerEntity->y) + world->random().nextInt(16) - world->random().nextInt(16);
   if(y < 1) {
    y = 1;
   } else if(y > 128) {
    y = 128;
   }
   const int monsterIndex = world->random().nextInt(static_cast<int>(MONSTER_TYPE.size()));
   const std::string& monsterId = MONSTER_TYPE[static_cast<std::size_t>(monsterIndex)];
   int groundY = y;
   for(; groundY > 2 && !world->shouldSuffocate(x, groundY - 1, z); --groundY) {
   }
   while(!isValidSpawnPos(kSpawnRules[0], world, x, groundY, z) && groundY < y + 16 && groundY < 128) {
    ++groundY;
   }
   if(groundY >= y + 16 || groundY >= 128) {
    groundY = y;
    continue;
   }
   const float fx = static_cast<float>(x) + 0.5f;
   const float fy = static_cast<float>(groundY);
   const float fz = static_cast<float>(z) + 0.5f;
   std::optional<Vec3i> wakePos;
   LivingEntity* living = world->spawnMob(monsterId, [&](LivingEntity& mob) {
    mob.setPositionAndAnglesKeepPrevAngles(fx, fy, fz, world->random().nextFloat() * 360.0f, 0.0f);
    if(!mob.canSpawn()) {
     return false;
    }
    entity::ai::pathing::Path path = pathNodeNavigator.findPath(&mob, playerEntity, 32.0f);
    if(path.length <= 1) {
     return false;
    }
    const entity::ai::pathing::PathNode* pathEnd = path.getEnd();
    if(pathEnd == nullptr) {
     return false;
    }
    if(std::abs(static_cast<double>(pathEnd->x) - playerEntity->x) >= 1.5 ||
       std::abs(static_cast<double>(pathEnd->z) - playerEntity->z) >= 1.5 ||
       std::abs(static_cast<double>(pathEnd->y) - playerEntity->y) >= 1.5) {
     return false;
    }
    wakePos = block::BedBlock::findWakeUpPosition(world,
                                                  MathHelper::floor(playerEntity->x),
                                                  MathHelper::floor(playerEntity->y),
                                                  MathHelper::floor(playerEntity->z),
                                                  1);
    if(!wakePos.has_value()) {
     wakePos = Vec3i{x, groundY + 1, z};
    }
    mob.setPositionAndAnglesKeepPrevAngles(static_cast<float>(wakePos->x) + 0.5f,
                                           static_cast<float>(wakePos->y),
                                           static_cast<float>(wakePos->z) + 0.5f,
                                           0.0f,
                                           0.0f);
    return true;
   });
   if(living == nullptr || !wakePos.has_value()) {
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
} // namespace net::minecraft
