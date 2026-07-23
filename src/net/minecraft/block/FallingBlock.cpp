#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block {
thread_local bool FallingBlock::fallInstantly = false;
void FallingBlock::onPlaced(World* world, int x, int y, int z) {
 if(world != nullptr) {
  world->scheduleBlockUpdate(x, y, z, this->id, getTickRate());
 }
}
void FallingBlock::neighborUpdate(World* world, int x, int y, int z, int /*neighborId*/) {
 if(world != nullptr) {
  world->scheduleBlockUpdate(x, y, z, this->id, getTickRate());
 }
}
void FallingBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) {
 processFall(world, x, y, z);
}
bool FallingBlock::canFallThrough(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return false;
 }
 if(!world->isRegionLoaded(x, y, z, 1)) {
  return false;
 }
 const int blockId = world->getBlockId(x, y, z);
 if(blockId == 0) {
  return true;
 }
 if(Block::FIRE != nullptr && blockId == Block::FIRE->id) {
  return true;
 }
 Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
 if(block == nullptr) {
  return false;
 }
 if(&block->material == &material::Material::WATER) {
  return true;
 }
 return &block->material == &material::Material::LAVA;
}
void FallingBlock::processFall(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return;
 }
 int fallX = x;
 int fallY = y;
 int fallZ = z;
 if(!canFallThrough(world, fallX, fallY - 1, fallZ) || fallY < 0) {
  return;
 }
 constexpr int regionRadius = 32;
 if(fallInstantly || !world->isRegionLoaded(fallX - regionRadius,
                                            fallY - regionRadius,
                                            fallZ - regionRadius,
                                            fallX + regionRadius,
                                            fallY + regionRadius,
                                            fallZ + regionRadius)) {
  world->setBlock(fallX, fallY, fallZ, 0);
  while(canFallThrough(world, fallX, fallY - 1, fallZ) && fallY > 0) {
   --fallY;
  }
  if(fallY > 0) {
   world->setBlock(fallX, fallY, fallZ, id);
  }
  return;
 }
 auto* falling = new FallingBlockEntity(world,
                                        static_cast<double>(fallX) + 0.5,
                                        static_cast<double>(fallY) + 0.5,
                                        static_cast<double>(fallZ) + 0.5,
                                        id);
 world->spawnEntity(falling);
}
} // namespace net::minecraft::block
