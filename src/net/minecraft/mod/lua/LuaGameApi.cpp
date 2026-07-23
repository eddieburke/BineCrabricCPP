#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/block/Block.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/particle/ParticleManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#endif
#include <cctype>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::lua {
namespace detail {
[[nodiscard]] int vanillaBlockIdByToken(const char* token) {
 if(token == nullptr) {
  return 0;
 }
 std::string name = token;
 for(char& ch : name) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 }
 static const std::unordered_map<std::string, int> kIds = {
     {"air", 0},
     {"stone", 1},
     {"grass", 2},
     {"grass_block", 2},
     {"dirt", 3},
     {"cobblestone", 4},
     {"planks", 5},
     {"sapling", 6},
     {"bedrock", 7},
     {"water", 8},
     {"flowing_water", 8},
     {"still_water", 9},
     {"lava", 10},
     {"flowing_lava", 10},
     {"still_lava", 11},
     {"sand", 12},
     {"gravel", 13},
     {"gold_ore", 14},
     {"iron_ore", 15},
     {"coal_ore", 16},
     {"log", 17},
     {"leaves", 18},
     {"sponge", 19},
     {"glass", 20},
     {"lapis_ore", 21},
     {"lapis_block", 22},
     {"sandstone", 24},
     {"cobweb", 30},
     {"dispenser", 23},
     {"note_block", 25},
     {"bed", 26},
     {"powered_rail", 27},
     {"detector_rail", 28},
     {"sticky_piston", 29},
     {"tall_grass", 31},
     {"dead_bush", 32},
     {"piston", 33},
     {"piston_head", 34},
     {"wool", 35},
     {"moving_piston", 36},
     {"dandelion", 37},
     {"rose", 38},
     {"brown_mushroom", 39},
     {"red_mushroom", 40},
     {"gold_block", 41},
     {"iron_block", 42},
     {"double_slab", 43},
     {"slab", 44},
     {"bricks", 45},
     {"tnt", 46},
     {"bookshelf", 47},
     {"mossy_cobblestone", 48},
     {"obsidian", 49},
     {"torch", 50},
     {"fire", 51},
     {"mob_spawner", 52},
     {"wood_stairs", 53},
     {"chest", 54},
     {"redstone_wire", 55},
     {"diamond_ore", 56},
     {"diamond_block", 57},
     {"workbench", 58},
     {"crops", 59},
     {"farmland", 60},
     {"furnace", 61},
     {"lit_furnace", 62},
     {"standing_sign", 63},
     {"wooden_door", 64},
     {"ladder", 65},
     {"rail", 66},
     {"cobblestone_stairs", 67},
     {"wall_sign", 68},
     {"lever", 69},
     {"stone_pressure_plate", 70},
     {"iron_door", 71},
     {"wooden_pressure_plate", 72},
     {"redstone_ore", 73},
     {"glowing_redstone_ore", 74},
     {"redstone_torch_off", 75},
     {"redstone_torch", 76},
     {"button", 77},
     {"snow_layer", 78},
     {"snow", 78},
     {"ice", 79},
     {"snow_block", 80},
     {"cactus", 81},
     {"clay", 82},
     {"sugar_cane", 83},
     {"jukebox", 84},
     {"fence", 85},
     {"pumpkin", 86},
     {"netherrack", 87},
     {"soul_sand", 88},
     {"glowstone", 89},
     {"portal", 90},
     {"jack_o_lantern", 91},
     {"cake", 92},
     {"repeater_off", 93},
     {"repeater", 94},
     {"locked_chest", 95},
     {"trapdoor", 96},
 };
 const auto it = kIds.find(name);
 return it == kIds.end() ? 0 : it->second;
}
} // namespace detail
int blockIdFromName(const char* name) {
 if(name == nullptr || *name == '\0') {
  return 0;
 }
 const int vanilla = detail::vanillaBlockIdByToken(name);
 if(vanilla != 0) {
  return vanilla;
 }
 return modBlockIdFromName(name);
}
std::string blockWireNameFromId(int blockId) {
 if(blockId > 0 && blockId < block::Block::BLOCK_COUNT &&
    block::Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
  std::string key = block::Block::BLOCKS[static_cast<std::size_t>(blockId)]->getTranslationKey();
  if(key.rfind("tile.", 0) == 0) {
   key = key.substr(5);
  }
  return key;
 }
 return modBlockWireName(blockId);
}
bool worldIsNight(const World* world) {
 if(world == nullptr) {
  return false;
 }
 const std::uint64_t time = world->getTime() % 24000ULL;
 return time > 13000ULL && time < 23000ULL;
}
int worldRandomInt(World* world, int bound) {
 if(bound <= 0 || world == nullptr) {
  return 0;
 }
 return world->random().nextInt(bound);
}
bool spawnEntityByName(World* world, const char* entityId, double x, double y, double z) {
 if(world == nullptr || entityId == nullptr || *entityId == '\0') {
  return false;
 }
 std::unique_ptr<entity::Entity> entity = entity::EntityRegistry::create(entityId, world);
 if(entity == nullptr) {
  return false;
 }
 entity->setPosition(x, y, z);
 world->spawnEntity(entity.release());
 return true;
}
int countEntitiesByName(const World* world, const char* entityId) {
 if(world == nullptr || entityId == nullptr || *entityId == '\0') {
  return 0;
 }
 int count = 0;
 for(const entity::Entity* entity : world->entities()) {
  if(entity != nullptr && entity::EntityRegistry::getId(*entity) == entityId) {
   ++count;
  }
 }
 return count;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
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
                         float gravity) {
 client::Minecraft* client = client::Minecraft::INSTANCE;
 if(client == nullptr || client->world == nullptr) {
  return false;
 }
 auto* particle = new client::particle::Particle(client->world, x, y, z, vx, vy, vz);
 particle->scale = std::clamp(scale, 0.05f, 4.0f);
 particle->red = red;
 particle->green = green;
 particle->blue = blue;
 particle->maxParticleAge = maxAge;
 particle->gravityStrength = gravity;
 particle->velocityX = vx;
 particle->velocityY = vy;
 particle->velocityZ = vz;
 client->particleManager.addParticle(particle);
 return true;
}
bool readPlayerPosition(double& x, double& y, double& z) {
 client::Minecraft* client = client::Minecraft::INSTANCE;
 if(client == nullptr || client->player == nullptr) {
  return false;
 }
 x = client->player->x;
 y = client->player->y;
 z = client->player->z;
 return true;
}
#else
bool spawnClientParticle(double /*x*/,
                         double /*y*/,
                         double /*z*/,
                         double /*vx*/,
                         double /*vy*/,
                         double /*vz*/,
                         float /*scale*/,
                         float /*red*/,
                         float /*green*/,
                         float /*blue*/,
                         int /*maxAge*/,
                         float /*gravity*/) {
 return false;
}
bool readPlayerPosition(double& /*x*/, double& /*y*/, double& /*z*/) {
 return false;
}
#endif
int getBlockIdAt(World* world, int x, int y, int z) {
 return world != nullptr ? world->getBlockId(x, y, z) : 0;
}
float normalizedCelestial(const World* world, float tickDelta) {
 if(world == nullptr) {
  return 0.0f;
 }
 double time = world->getTime(tickDelta);
 time = std::fmod(time, 24000.0);
 if(time < 0.0) {
  time += 24000.0;
 }
 return static_cast<float>(time / 24000.0);
}
} // namespace net::minecraft::mod::lua
