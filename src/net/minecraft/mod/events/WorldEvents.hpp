#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class Chunk;
class ChunkSource;
class World;
namespace block {
class Block;
}
} // namespace net::minecraft
namespace net::minecraft::mod {
struct WorldTickEvent {
 World* world = nullptr;
 bool remote = false; // World::isRemote(): a replica fed by packets, not the simulating side
 bool before = true;
};
struct WorldTimeEvent {
 World* world = nullptr;
 long long oldTime = 0;
 long long newTime = 0;
 bool canceled = false;
};
struct WeatherCycleEvent {
 World* world = nullptr;
 bool remote = false;
 bool canceled = false;
};
struct LightningStrikeEvent {
 World* world = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 bool canceled = false;
};
struct SnowIcePlacementEvent {
 World* world = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 bool placeSnow = false;
 bool placeIce = false;
};
struct RandomBlockTickEvent {
 World* world = nullptr;
 block::Block* block = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 int blockId = 0;
 bool canceled = false;
};
struct ScheduledBlockTickEvent {
 World* world = nullptr;
 block::Block* block = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 int blockId = 0;
 bool instant = false;
 bool canceled = false;
};
struct ScheduleBlockUpdateEvent {
 World* world = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 int blockId = 0;
 int tickRate = 0;
 bool canceled = false;
};
struct TickRateEvent {
 float targetTps = 20.0f;
 float tpsScale = 1.0f;
};
struct ChunkDecorateEvent {
 World* world = nullptr;
 int chunkX = 0;
 int chunkZ = 0;
 bool before = true;
 bool canceled = false;
 Chunk* chunk = nullptr;
 ChunkSource* source = nullptr;
};
struct CreateWorldEvent {
 const std::string* saveName = nullptr;
 std::int64_t seed = 0;
 bool canceled = false;
 std::unordered_map<std::string, std::string> options;
};
struct WorldOpenEvent {
 const std::string* saveName = nullptr;
 bool newWorld = false;
 const std::unordered_map<std::string, std::string>* options = nullptr;
};
struct WorldStartEvent {
 World* world = nullptr;
 const std::string* saveName = nullptr;
 bool newWorld = false;
};
struct WorldSpawnSearchEvent {
 World* world = nullptr;
 int x = 0;
 int y = 64;
 int z = 0;
 bool resolved = false;
};
} // namespace net::minecraft::mod
