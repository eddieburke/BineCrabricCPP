#pragma once
#include "net/minecraft/block/BlockTypes.hpp"
#include "net/minecraft/sound/BlockSoundGroup.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
namespace net::minecraft {
class World;
} // namespace net::minecraft
namespace net::minecraft::block {
// Beta's shared Block.soundWoodFootstep / soundGravelFootstep / ... Each block .cpp
// used to re-declare the one it needed in its own anonymous namespace, so every
// translation unit built a separate object with identical state — kWoodSound alone
// existed 18 times. setSoundGroup takes a mutable pointer, so these are not const.
inline net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
inline net::minecraft::BlockSoundGroup kDirtSound("grass", 1.0f, 1.0f);
inline net::minecraft::BlockSoundGroup kMetalSound("stone", 1.0f, 1.5f);
inline net::minecraft::BlockSoundGroup kClothSound("cloth", 1.0f, 1.0f);
inline net::minecraft::BlockSoundGroup kGravelSound("gravel", 1.0f, 1.0f);
} // namespace net::minecraft::block
namespace net::minecraft::block::sounds {
void playStep(World* world, Entity* entity, Block* block);
void playMining(World* world, double x, double y, double z, Block* block);
void playBreak(World* world, double x, double y, double z, Block* block);
void playPlace(World* world, double x, double y, double z, Block* block);
void playLanding(World* world, Entity* entity, Block* block);
} // namespace net::minecraft::block::sounds
