#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft {
class World;
struct SpawnRules {
 int creatureType = 0;
 int capacity = 0;
 bool waterSpawn = false;
 bool peaceful = false;
};
[[nodiscard]] bool isValidSpawnPos(const SpawnRules& rules, World* world, int x, int y, int z);
class NaturalSpawner {
 public:
 static Vec3i getRandomPosInChunk(World* world, int chunkX, int chunkZ);
 static int tick(World* world, bool spawnHostile, bool spawnPeaceful);
 static bool spawnMonstersAndWakePlayers(World* world, std::vector<entity::player::PlayerEntity*>& players);

 protected:
 static inline std::vector<std::string> MONSTER_TYPE = {"Spider", "Zombie", "Skeleton"};

 private:
 static inline std::unordered_set<ChunkPos, ChunkPosHash> mobSpawningChunks_;
};
} // namespace net::minecraft
