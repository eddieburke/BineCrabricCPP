#include "net/minecraft/block/BlockSounds.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block::sounds {
void playStep(World* world, Entity* entity, Block* block) {
  if(world == nullptr || entity == nullptr || block == nullptr) {
    return;
  }
  const BlockSoundGroup* group = block->getSoundGroup();
  world->playSound(entity, block->getStepSound(), group->getVolume() * 0.15f, group->getPitch());
}
void playMining(World* world, double x, double y, double z, Block* block) {
  if(world == nullptr || block == nullptr) {
    return;
  }
  const BlockSoundGroup* group = block->getSoundGroup();
  world->playSound(x, y, z, block->getMiningSound(), (group->getVolume() + 1.0f) / 8.0f, group->getPitch() * 0.5f);
}
void playBreak(World* world, double x, double y, double z, Block* block) {
  if(world == nullptr || block == nullptr) {
    return;
  }
  const BlockSoundGroup* group = block->getSoundGroup();
  world->playSound(x, y, z, block->getBreakSound(), (group->getVolume() + 1.0f) / 2.0f, group->getPitch() * 0.8f);
}
void playPlace(World* world, double x, double y, double z, Block* block) {
  if(world == nullptr || block == nullptr) {
    return;
  }
  const BlockSoundGroup* group = block->getSoundGroup();
  world->playSound(x, y, z, block->getStepSound(), (group->getVolume() + 1.0f) * 0.5f, group->getPitch() * 0.8f);
}
void playLanding(World* world, Entity* entity, Block* block) {
  if(world == nullptr || entity == nullptr || block == nullptr) {
    return;
  }
  const BlockSoundGroup* group = block->getSoundGroup();
  world->playSound(entity, block->getStepSound(), group->getVolume() * 0.5f, group->getPitch() * 0.75f);
}
} // namespace net::minecraft::block::sounds
