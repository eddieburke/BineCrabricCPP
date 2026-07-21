#include "net/minecraft/block/PistonExtensionBlock.hpp"
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block {
namespace {
entity::PistonBlockEntity* getPistonBlockEntity(const BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return nullptr;
 }
 return dynamic_cast<entity::PistonBlockEntity*>(const_cast<BlockView*>(blockView)->getBlockEntity(x, y, z));
}
} // namespace
std::unique_ptr<entity::BlockEntity> PistonExtensionBlock::createBlockEntity() {
 return nullptr;
}
std::unique_ptr<entity::BlockEntity> PistonExtensionBlock::createPistonBlockEntity(
    int blockId, int blockMeta, int facing, bool extending, bool source) {
 return std::make_unique<entity::PistonBlockEntity>(blockId, blockMeta, facing, extending, source);
}
bool PistonExtensionBlock::canPlaceAt(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const {
 return false;
}
bool PistonExtensionBlock::canPlaceAt(World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*side*/) const {
 return false;
}
std::optional<net::minecraft::Box> PistonExtensionBlock::getCollisionShape(World* world, int x, int y, int z) const {
 entity::PistonBlockEntity* piston = getPistonBlockEntity(world, x, y, z);
 if(piston == nullptr) {
  return std::nullopt;
 }
 float progress = piston->getProgress(0.0f);
 if(piston->isExtending()) {
  progress = 1.0f - progress;
 }
 return getPushedBlockCollisionShape(world, x, y, z, piston->getPushedBlockId(), progress, piston->getFacing());
}
void PistonExtensionBlock::onBreak(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return;
 }
 if(auto* piston = dynamic_cast<entity::PistonBlockEntity*>(world->getBlockEntity(x, y, z)); piston != nullptr) {
  piston->finish();
  return;
 }
 BlockWithEntity::onBreak(world, x, y, z);
}
bool PistonExtensionBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/) {
 if(world == nullptr || world->isRemote() || world->getBlockEntity(x, y, z) != nullptr) {
  return false;
 }
 world->setBlock(x, y, z, 0);
 return true;
}
void PistonExtensionBlock::dropStacks(World* world, int x, int y, int z, int /*meta*/, float /*luck*/) {
 if(world == nullptr || world->isRemote()) {
  return;
 }
 entity::PistonBlockEntity* piston = getPistonBlockEntity(world, x, y, z);
 if(piston == nullptr) {
  return;
 }
 const int pushedBlockId = piston->getPushedBlockId();
 if(pushedBlockId <= 0 || pushedBlockId >= Block::BLOCK_COUNT) {
  return;
 }
 Block* pushedBlock = Block::BLOCKS[static_cast<std::size_t>(pushedBlockId)];
 if(pushedBlock != nullptr) {
  pushedBlock->dropStacks(world, x, y, z, piston->getPushedBlockData());
 }
}
void PistonExtensionBlock::neighborUpdate(World* world, int x, int y, int z, int /*neighborId*/) {
 if(world == nullptr || world->isRemote() || world->getBlockEntity(x, y, z) != nullptr) {
  return;
 }
}
void PistonExtensionBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
 setBoundingBox(getRenderBounds(blockView, x, y, z));
}
net::minecraft::Box PistonExtensionBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
 const net::minecraft::Box current{minX, minY, minZ, maxX, maxY, maxZ};
 entity::PistonBlockEntity* piston = getPistonBlockEntity(blockView, x, y, z);
 if(piston == nullptr) {
  return current;
 }
 const int pushedBlockId = piston->getPushedBlockId();
 if(pushedBlockId <= 0 || pushedBlockId >= Block::BLOCK_COUNT) {
  return current;
 }
 Block* pushedBlock = Block::BLOCKS[static_cast<std::size_t>(pushedBlockId)];
 if(pushedBlock == nullptr || pushedBlock == this) {
  return current;
 }
 const net::minecraft::Box pushed = pushedBlock->getRenderBounds(blockView, x, y, z);
 float progress = piston->getProgress(0.0f);
 if(piston->isExtending()) {
  progress = 1.0f - progress;
 }
 const int facing = piston->getFacing();
 if(facing < 0 || facing >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
  return current;
 }
 const float offsetX =
     static_cast<float>(PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)]) * progress;
 const float offsetY =
     static_cast<float>(PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)]) * progress;
 const float offsetZ =
     static_cast<float>(PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)]) * progress;
 return pushed.offset(-static_cast<double>(offsetX), -static_cast<double>(offsetY), -static_cast<double>(offsetZ));
}
std::optional<net::minecraft::Box> PistonExtensionBlock::getPushedBlockCollisionShape(
    World* world, int x, int y, int z, int blockId, float sizeMultiplier, int facing) const {
 if(blockId == 0 || blockId == id) {
  return std::nullopt;
 }
 if(blockId < 0 || blockId >= Block::BLOCK_COUNT) {
  return std::nullopt;
 }
 Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
 if(block == nullptr) {
  return std::nullopt;
 }
 std::optional<net::minecraft::Box> box = block->getCollisionShape(world, x, y, z);
 if(!box.has_value()) {
  return std::nullopt;
 }
 if(facing < 0 || facing >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
  return box;
 }
 const float offsetX =
     static_cast<float>(PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)]) * sizeMultiplier;
 const float offsetY =
     static_cast<float>(PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)]) * sizeMultiplier;
 const float offsetZ =
     static_cast<float>(PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)]) * sizeMultiplier;
 box->minX -= offsetX;
 box->maxX -= offsetX;
 box->minY -= offsetY;
 box->maxY -= offsetY;
 box->minZ -= offsetZ;
 box->maxZ -= offsetZ;
 return box;
}
void PistonExtensionBlock::registerClass() {
 Block::MOVING_PISTON = new PistonExtensionBlock(kBlockId);
}
namespace {
} // namespace
MC_REGISTER_BLOCK(PistonExtensionBlock)
} // namespace net::minecraft::block
