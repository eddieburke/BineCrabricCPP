#pragma once
#include <string>
struct lua_State;
namespace net::minecraft {
class World;
namespace entity::player {
class PlayerEntity;
}
} // namespace net::minecraft
namespace net::minecraft::mod::lua {
int blockIdFromName(const char* name);
std::string blockWireNameFromId(int blockId);
bool worldIsNight(const World* world);
int worldRandomInt(World* world, int bound);
bool spawnEntityByName(World* world, const char* entityId, double x, double y, double z);
int countEntitiesByName(const World* world, const char* entityId);
bool spawnClientParticle(double x,
                         double y,
                         double z,
                         double vx,
                         double vy,
                         double vz,
                         float scale,
                         float red,
                         float green,
                         float blue,
                         int maxAge,
                         float gravity);
entity::player::PlayerEntity* localPlayer();
int getBlockIdAt(World* world, int x, int y, int z);
} // namespace net::minecraft::mod::lua
