#include "net/minecraft/block/NetherPortalBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block {
std::optional<Box> NetherPortalBlock::getCollisionShape(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const {
 return std::nullopt;
}
void NetherPortalBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
 setBoundingBox(getRenderBounds(blockView, x, y, z));
}
net::minecraft::Box NetherPortalBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
 if(blockView == nullptr) {
  return {minX, minY, minZ, maxX, maxY, maxZ};
 }
 if(blockView->getBlockId(x - 1, y, z) == id || blockView->getBlockId(x + 1, y, z) == id) {
  constexpr float halfWidth = 0.5f;
  constexpr float halfDepth = 0.125f;
  return {0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth};
 }
 constexpr float halfWidth = 0.125f;
 constexpr float halfDepth = 0.5f;
 return {0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth};
}
bool NetherPortalBlock::create(World* world, int x, int y, int z) {
 if(world == nullptr || Block::OBSIDIAN == nullptr) {
  return false;
 }
 int axisX = 0;
 int axisZ = 0;
 if(world->getBlockId(x - 1, y, z) == Block::OBSIDIAN->id ||
    world->getBlockId(x + 1, y, z) == Block::OBSIDIAN->id) {
  axisX = 1;
 }
 if(world->getBlockId(x, y, z - 1) == Block::OBSIDIAN->id ||
    world->getBlockId(x, y, z + 1) == Block::OBSIDIAN->id) {
  axisZ = 1;
 }
 if(axisX == axisZ) {
  return false;
 }
 if(world->getBlockId(x - axisX, y, z - axisZ) == 0) {
  x -= axisX;
  z -= axisZ;
 }
 for(int dx = -1; dx <= 2; ++dx) {
  for(int dy = -1; dy <= 3; ++dy) {
   if((dx == -1 || dx == 2) && (dy == -1 || dy == 3)) {
    continue;
   }
   const bool frame = dx == -1 || dx == 2 || dy == -1 || dy == 3;
   const int blockId = world->getBlockId(x + axisX * dx, y + dy, z + axisZ * dx);
   if(frame) {
    if(blockId != Block::OBSIDIAN->id) {
     return false;
    }
   } else if(blockId != 0 && (Block::FIRE == nullptr || blockId != Block::FIRE->id)) {
    return false;
   }
  }
 }
 world->pauseTicking = true;
 for(int dx = 0; dx < 2; ++dx) {
  for(int dy = 0; dy < 3; ++dy) {
   world->setBlock(x + axisX * dx, y + dy, z + axisZ * dx, id);
  }
 }
 world->pauseTicking = false;
 return true;
}
void NetherPortalBlock::neighborUpdate(World* world, int x, int y, int z, int /*blockId*/) {
 if(world == nullptr || Block::OBSIDIAN == nullptr) {
  return;
 }
 int axisX = 0;
 int axisZ = 1;
 if(world->getBlockId(x - 1, y, z) == id || world->getBlockId(x + 1, y, z) == id) {
  axisX = 1;
  axisZ = 0;
 }
 int baseY = y;
 while(world->getBlockId(x, baseY - 1, z) == id) {
  --baseY;
 }
 if(world->getBlockId(x, baseY - 1, z) != Block::OBSIDIAN->id) {
  world->setBlock(x, y, z, 0);
  return;
 }
 int height = 1;
 while(height < 4 && world->getBlockId(x, baseY + height, z) == id) {
  ++height;
 }
 if(height != 3 || world->getBlockId(x, baseY + height, z) != Block::OBSIDIAN->id) {
  world->setBlock(x, y, z, 0);
  return;
 }
 const bool horizontalPortal = world->getBlockId(x - 1, y, z) == id || world->getBlockId(x + 1, y, z) == id;
 const bool depthPortal = world->getBlockId(x, y, z - 1) == id || world->getBlockId(x, y, z + 1) == id;
 if(horizontalPortal && depthPortal) {
  world->setBlock(x, y, z, 0);
  return;
 }
 const bool validFrame = (world->getBlockId(x + axisX, y, z + axisZ) == Block::OBSIDIAN->id &&
                          world->getBlockId(x - axisX, y, z - axisZ) == id) ||
                         (world->getBlockId(x - axisX, y, z - axisZ) == Block::OBSIDIAN->id &&
                          world->getBlockId(x + axisX, y, z + axisZ) == id);
 if(!validFrame) {
  world->setBlock(x, y, z, 0);
 }
}
bool NetherPortalBlock::isSideVisibleForBounds(
    const BlockView* blockView, int x, int y, int z, int side, const net::minecraft::Box& /*bounds*/) const {
 if(blockView == nullptr) {
  return false;
 }
 if(blockView->getBlockId(x, y, z) == id) {
  return false;
 }
 const bool westEdge = blockView->getBlockId(x - 1, y, z) == id && blockView->getBlockId(x - 2, y, z) != id;
 const bool eastEdge = blockView->getBlockId(x + 1, y, z) == id && blockView->getBlockId(x + 2, y, z) != id;
 const bool northEdge = blockView->getBlockId(x, y, z - 1) == id && blockView->getBlockId(x, y, z - 2) != id;
 const bool southEdge = blockView->getBlockId(x, y, z + 1) == id && blockView->getBlockId(x, y, z + 2) != id;
 const bool horizontal = westEdge || eastEdge;
 const bool depth = northEdge || southEdge;
 if(horizontal && (side == 4 || side == 5)) {
  return true;
 }
 return depth && (side == 2 || side == 3);
}
void NetherPortalBlock::onEntityCollision(
    World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::Entity* entity) {
 if(entity != nullptr && entity->vehicle == nullptr && entity->passenger == nullptr) {
  entity->tickPortalCooldown();
 }
}
void NetherPortalBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) {
 if(world == nullptr) {
  return;
 }
 if(random.nextInt(100) == 0) {
  world->playSound(static_cast<double>(x) + 0.5,
                   static_cast<double>(y) + 0.5,
                   static_cast<double>(z) + 0.5,
                   "portal.portal",
                   1.0f,
                   random.nextFloat() * 0.4f + 0.8f);
 }
 for(int i = 0; i < 4; ++i) {
  double px = static_cast<double>(x) + random.nextFloat();
  double py = static_cast<double>(y) + random.nextFloat();
  double pz = static_cast<double>(z) + random.nextFloat();
  double vx = 0.0;
  double vy = 0.0;
  double vz = 0.0;
  const int sign = random.nextInt(2) * 2 - 1;
  vx = (static_cast<double>(random.nextFloat()) - 0.5) * 0.5;
  vy = (static_cast<double>(random.nextFloat()) - 0.5) * 0.5;
  vz = (static_cast<double>(random.nextFloat()) - 0.5) * 0.5;
  if(world->getBlockId(x - 1, y, z) == id || world->getBlockId(x + 1, y, z) == id) {
   pz = static_cast<double>(z) + 0.5 + 0.25 * static_cast<double>(sign);
   vz = random.nextFloat() * 2.0f * static_cast<float>(sign);
  } else {
   px = static_cast<double>(x) + 0.5 + 0.25 * static_cast<double>(sign);
   vx = random.nextFloat() * 2.0f * static_cast<float>(sign);
  }
  world->addParticle("portal", px, py, pz, vx, vy, vz);
 }
}
void NetherPortalBlock::registerClass() {
 Block::NETHER_PORTAL = (new NetherPortalBlock(kBlockId, 14))
                            ->setHardness(-1.0f)
                            ->setSoundGroup(&Block::GLASS_BLOCK_SOUNDS)
                            ->setLuminance(0.75f)
                            ->setLightColor(0.6f, 0.1f, 0.9f)
                            ->setTranslationKey("portal");
}
MC_REGISTER_BLOCK(NetherPortalBlock)
} // namespace net::minecraft::block
