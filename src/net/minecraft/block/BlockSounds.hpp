#pragma once
#include "net/minecraft/block/BlockTypes.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"

namespace net::minecraft {
class World;
}  // namespace net::minecraft

namespace net::minecraft::block::sounds {
void playStep(World* world, Entity* entity, Block* block);
void playMining(World* world, double x, double y, double z, Block* block);
void playBreak(World* world, double x, double y, double z, Block* block);
void playPlace(World* world, double x, double y, double z, Block* block);
void playLanding(World* world, Entity* entity, Block* block);
}  // namespace net::minecraft::block::sounds
