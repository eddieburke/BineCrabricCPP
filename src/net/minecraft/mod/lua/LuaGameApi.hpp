#pragma once
#include <string>
struct lua_State;
namespace net::minecraft {
class World;
}
namespace net::minecraft::mod::lua {
int blockIdFromName(const char* name);
#ifdef MINECRAFT_NATIVE_EXPORTS
std::string blockWireNameFromId(int blockId);
#endif
bool worldIsNight(const World* world);
int worldRandomInt(World* world, int bound);
bool spawnEntityByName(World* world, const char* entityId, double x, double y, double z);
int countEntitiesByName(const World* world, const char* entityId);
bool spawnClientParticle(double x, double y, double z, double vx, double vy, double vz, float scale, float red,
                         float green, float blue, int maxAge, float gravity);
bool readPlayerPosition(double& x, double& y, double& z);
int getBlockIdAt(World* world, int x, int y, int z);
float normalizedCelestial(const World* world, float tickDelta);
bool setPlayerCursorItem(int itemId, int count);
} // namespace net::minecraft::mod::lua
