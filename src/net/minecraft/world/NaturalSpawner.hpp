#pragma once

#include "net/minecraft/util/math/Types.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft {

class World;

class NaturalSpawner {
public:
    static Vec3i getRandomPosInChunk(World* world, int chunkX, int chunkZ);
    static int tick(World* world, bool spawnAnimals, bool spawnMonsters);
    static bool spawnMonstersAndWakePlayers(World* world, std::vector<entity::player::PlayerEntity*>& players);

protected:
    static inline std::vector<std::string> MONSTER_TYPE = {"Spider", "Zombie", "Skeleton"};

private:
    static inline std::unordered_set<ChunkPos, ChunkPosHash> mobSpawningChunks_;
};

} // namespace net::minecraft
