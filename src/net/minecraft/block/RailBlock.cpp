#include "net/minecraft/block/RailBlock.hpp"
#include <memory>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace {
net::minecraft::BlockSoundGroup kMetalSound("stone", 1.0f, 1.5f);
}
namespace net::minecraft::block {
namespace {
struct ConnectionPos {
 int x = 0;
 int y = 0;
 int z = 0;
};
class RailNode {
 public:
 RailNode(World* worldIn, int xIn, int yIn, int zIn) : world(worldIn), x(xIn), y(yIn), z(zIn) {
  const int blockId = world->getBlockId(x, y, z);
  int meta = world->getBlockMeta(x, y, z);
  auto* rail = dynamic_cast<RailBlock*>(Block::BLOCKS[static_cast<std::size_t>(blockId)]);
  if(rail != nullptr && rail->alwaysStraight) {
   alwaysStraight = true;
   meta &= 7;
  } else {
   alwaysStraight = false;
  }
  updateConnections(meta);
 }
 [[nodiscard]] int countConnections() const {
  int count = 0;
  if(couldConnectTo(x, y, z - 1)) {
   ++count;
  }
  if(couldConnectTo(x, y, z + 1)) {
   ++count;
  }
  if(couldConnectTo(x - 1, y, z)) {
   ++count;
  }
  if(couldConnectTo(x + 1, y, z)) {
   ++count;
  }
  return count;
 }
 void updateState(bool powered, bool force) {
  bool north = hasNeighborRail(x, y, z - 1);
  bool south = hasNeighborRail(x, y, z + 1);
  bool west = hasNeighborRail(x - 1, y, z);
  bool east = hasNeighborRail(x + 1, y, z);
  int shape = -1;
  if((north || south) && !west && !east) {
   shape = 0;
  }
  if((west || east) && !north && !south) {
   shape = 1;
  }
  if(!alwaysStraight) {
   if(south && east && !north && !west) {
    shape = 6;
   }
   if(south && west && !north && !east) {
    shape = 7;
   }
   if(north && west && !south && !east) {
    shape = 8;
   }
   if(north && east && !south && !west) {
    shape = 9;
   }
  }
  if(shape == -1) {
   if(north || south) {
    shape = 0;
   }
   if(west || east) {
    shape = 1;
   }
   if(!alwaysStraight) {
    if(powered) {
     if(south && east) {
      shape = 6;
     }
     if(west && south) {
      shape = 7;
     }
     if(east && north) {
      shape = 9;
     }
     if(north && west) {
      shape = 8;
     }
    } else {
     if(north && west) {
      shape = 8;
     }
     if(east && north) {
      shape = 9;
     }
     if(west && south) {
      shape = 7;
     }
     if(south && east) {
      shape = 6;
     }
    }
   }
  }
  if(shape == 0) {
   if(RailBlock::isRail(world, x, y + 1, z - 1)) {
    shape = 4;
   }
   if(RailBlock::isRail(world, x, y + 1, z + 1)) {
    shape = 5;
   }
  }
  if(shape == 1) {
   if(RailBlock::isRail(world, x + 1, y + 1, z)) {
    shape = 2;
   }
   if(RailBlock::isRail(world, x - 1, y + 1, z)) {
    shape = 3;
   }
  }
  if(shape < 0) {
   shape = 0;
  }
  updateConnections(shape);
  int newMeta = shape;
  if(alwaysStraight) {
   const int straightBit = world->getBlockMeta(x, y, z) & 8;
   newMeta = straightBit | shape;
  }
  if(force || world->getBlockMeta(x, y, z) != newMeta) {
   world->setBlockMeta(x, y, z, newMeta);
   for(const ConnectionPos& connection : connections) {
    std::unique_ptr<RailNode> neighbor{getNeighborRail(connection)};
    if(neighbor == nullptr) {
     continue;
    }
    neighbor->removeSoftConnections();
    if(!neighbor->canConnectTo(*this)) {
     continue;
    }
    neighbor->addConnection(*this);
   }
  }
 }

 private:
 World* world = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 bool alwaysStraight = false;
 std::vector<ConnectionPos> connections;
 void updateConnections(int meta) {
  connections.clear();
  switch(meta) {
  case 0:
   connections.emplace_back(x, y, z - 1);
   connections.emplace_back(x, y, z + 1);
   break;
  case 1:
   connections.emplace_back(x - 1, y, z);
   connections.emplace_back(x + 1, y, z);
   break;
  case 2:
   connections.emplace_back(x - 1, y, z);
   connections.emplace_back(x + 1, y + 1, z);
   break;
  case 3:
   connections.emplace_back(x - 1, y + 1, z);
   connections.emplace_back(x + 1, y, z);
   break;
  case 4:
   connections.emplace_back(x, y + 1, z - 1);
   connections.emplace_back(x, y, z + 1);
   break;
  case 5:
   connections.emplace_back(x, y, z - 1);
   connections.emplace_back(x, y + 1, z + 1);
   break;
  case 6:
   connections.emplace_back(x + 1, y, z);
   connections.emplace_back(x, y, z + 1);
   break;
  case 7:
   connections.emplace_back(x - 1, y, z);
   connections.emplace_back(x, y, z + 1);
   break;
  case 8:
   connections.emplace_back(x - 1, y, z);
   connections.emplace_back(x, y, z - 1);
   break;
  case 9:
   connections.emplace_back(x + 1, y, z);
   connections.emplace_back(x, y, z - 1);
   break;
  default:
   break;
  }
 }
 void removeSoftConnections() {
  for(std::size_t i = 0; i < connections.size(); ++i) {
   std::unique_ptr<RailNode> neighbor = getNeighborRail(connections[i]);
   if(neighbor == nullptr || !neighbor->connectsTo(*this)) {
    connections.erase(connections.begin() + static_cast<std::ptrdiff_t>(i));
    --i;
    continue;
   }
   connections[static_cast<std::size_t>(i)] = ConnectionPos{neighbor->x, neighbor->y, neighbor->z};
  }
 }
 [[nodiscard]] bool couldConnectTo(int cx, int cy, int cz) const {
  return RailBlock::isRail(world, cx, cy, cz) || RailBlock::isRail(world, cx, cy + 1, cz) ||
         RailBlock::isRail(world, cx, cy - 1, cz);
 }
 std::unique_ptr<RailNode> getNeighborRail(const ConnectionPos& pos) {
  if(RailBlock::isRail(world, pos.x, pos.y, pos.z)) {
   return std::make_unique<RailNode>(world, pos.x, pos.y, pos.z);
  }
  if(RailBlock::isRail(world, pos.x, pos.y + 1, pos.z)) {
   return std::make_unique<RailNode>(world, pos.x, pos.y + 1, pos.z);
  }
  if(RailBlock::isRail(world, pos.x, pos.y - 1, pos.z)) {
   return std::make_unique<RailNode>(world, pos.x, pos.y - 1, pos.z);
  }
  return nullptr;
 }
 [[nodiscard]] bool connectsTo(const RailNode& other) const {
  for(const ConnectionPos& connection : connections) {
   if(connection.x == other.x && connection.z == other.z) {
    return true;
   }
  }
  return false;
 }
 [[nodiscard]] bool hasConnection(int cx, int cz) const {
  for(const ConnectionPos& connection : connections) {
   if(connection.x == cx && connection.z == cz) {
    return true;
   }
  }
  return false;
 }
 [[nodiscard]] bool canConnectTo(const RailNode& other) const {
  if(connectsTo(other)) {
   return true;
  }
  return connections.size() != 2;
 }
 void addConnection(const RailNode& other) {
  connections.emplace_back(other.x, other.y, other.z);
  const bool north = hasConnection(x, z - 1);
  const bool south = hasConnection(x, z + 1);
  const bool west = hasConnection(x - 1, z);
  const bool east = hasConnection(x + 1, z);
  int shape = -1;
  if(north || south) {
   shape = 0;
  }
  if(west || east) {
   shape = 1;
  }
  if(!alwaysStraight) {
   if(south && east && !north && !west) {
    shape = 6;
   }
   if(south && west && !north && !east) {
    shape = 7;
   }
   if(north && west && !south && !east) {
    shape = 8;
   }
   if(north && east && !south && !west) {
    shape = 9;
   }
  }
  if(shape == 0) {
   if(RailBlock::isRail(world, x, y + 1, z - 1)) {
    shape = 4;
   }
   if(RailBlock::isRail(world, x, y + 1, z + 1)) {
    shape = 5;
   }
  }
  if(shape == 1) {
   if(RailBlock::isRail(world, x + 1, y + 1, z)) {
    shape = 2;
   }
   if(RailBlock::isRail(world, x - 1, y + 1, z)) {
    shape = 3;
   }
  }
  if(shape < 0) {
   shape = 0;
  }
  int newMeta = shape;
  if(alwaysStraight) {
   const int straightBit = world->getBlockMeta(x, y, z) & 8;
   newMeta = straightBit | shape;
  }
  world->setBlockMeta(x, y, z, newMeta);
 }
 bool hasNeighborRail(int cx, int cy, int cz) {
  std::unique_ptr<RailNode> neighbor{getNeighborRail(ConnectionPos{cx, cy, cz})};
  if(neighbor == nullptr) {
   return false;
  }
  neighbor->removeSoftConnections();
  return neighbor->canConnectTo(*this);
 }
};
} // namespace
RailBlock::RailBlock(int id, int textureId, bool alwaysStraightIn)
    : Block(id, textureId, material::Material::PISTON_BREAKABLE) {
 alwaysStraight = alwaysStraightIn;
 setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
}
bool RailBlock::isRail(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return false;
 }
 return isRail(world->getBlockId(x, y, z));
}
bool RailBlock::isRail(int blockId) {
 return ((Block::RAIL != nullptr && blockId == Block::RAIL->id) ||
         (Block::POWERED_RAIL != nullptr && blockId == Block::POWERED_RAIL->id) ||
         (Block::DETECTOR_RAIL != nullptr && blockId == Block::DETECTOR_RAIL->id));
}
int RailBlock::getTexture(int /*side*/, int meta) const {
 if(alwaysStraight ? Block::POWERED_RAIL != nullptr && id == Block::POWERED_RAIL->id && (meta & 8) == 0
                   : meta >= 6) {
  return textureId - 16;
 }
 return textureId;
}
bool RailBlock::canPlaceAt(World* world, int x, int y, int z) const {
 return world != nullptr && world->shouldSuffocate(x, y - 1, z);
}
void RailBlock::onPlaced(World* world, int x, int y, int z) {
 if(world != nullptr && !world->isRemote()) {
  updateShape(world, x, y, z, true);
 }
}
void RailBlock::neighborUpdate(World* world, int x, int y, int z, int neighborId) {
 if(world == nullptr || world->isRemote()) {
  return;
 }
 int meta = world->getBlockMeta(x, y, z);
 int shapeMeta = meta;
 if(alwaysStraight) {
  shapeMeta &= 7;
 }
 bool shouldBreak = false;
 if(!world->shouldSuffocate(x, y - 1, z)) {
  shouldBreak = true;
 }
 if(shapeMeta == 2 && !world->shouldSuffocate(x + 1, y, z)) {
  shouldBreak = true;
 }
 if(shapeMeta == 3 && !world->shouldSuffocate(x - 1, y, z)) {
  shouldBreak = true;
 }
 if(shapeMeta == 4 && !world->shouldSuffocate(x, y, z - 1)) {
  shouldBreak = true;
 }
 if(shapeMeta == 5 && !world->shouldSuffocate(x, y, z + 1)) {
  shouldBreak = true;
 }
 if(shouldBreak) {
  dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
  world->setBlock(x, y, z, 0);
  return;
 }
 if(Block::POWERED_RAIL != nullptr && id == Block::POWERED_RAIL->id) {
  bool powered = world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z);
  powered = powered || isPoweredByConnectedRails(world, x, y, z, meta, true, 0) ||
            isPoweredByConnectedRails(world, x, y, z, meta, false, 0);
  bool changed = false;
  if(powered && (meta & 8) == 0) {
   world->setBlockMeta(x, y, z, shapeMeta | 8);
   changed = true;
  } else if(!powered && (meta & 8) != 0) {
   world->setBlockMeta(x, y, z, shapeMeta);
   changed = true;
  }
  if(changed) {
   world->notifyNeighbors(x, y - 1, z, id);
   if(shapeMeta == 2 || shapeMeta == 3 || shapeMeta == 4 || shapeMeta == 5) {
    world->notifyNeighbors(x, y + 1, z, id);
   }
  }
 } else if(neighborId > 0 && neighborId < static_cast<int>(Block::BLOCKS.size())) {
  Block* neighbor = Block::BLOCKS[static_cast<std::size_t>(neighborId)];
  if(neighbor != nullptr && neighbor->canEmitRedstonePower() && !alwaysStraight) {
   RailNode node(world, x, y, z);
   if(node.countConnections() == 3) {
    updateShape(world, x, y, z, false);
   }
  }
 }
}
void RailBlock::updateShape(World* world, int x, int y, int z, bool force) {
 if(world == nullptr || world->isRemote()) {
  return;
 }
 RailNode node(world, x, y, z);
 node.updateState(world->isEmittingRedstonePower(x, y, z), force);
}
bool RailBlock::isPoweredByConnectedRails(
    World* world, int x, int y, int z, int meta, bool towardsNegative, int depth) {
 if(depth >= 8 || world == nullptr) {
  return false;
 }
 int shape = meta & 7;
 bool checkBelow = true;
 switch(shape) {
 case 0:
  if(towardsNegative) {
   ++z;
  } else {
   --z;
  }
  break;
 case 1:
  if(towardsNegative) {
   --x;
  } else {
   ++x;
  }
  break;
 case 2:
  if(towardsNegative) {
   --x;
  } else {
   ++x;
   ++y;
   checkBelow = false;
  }
  shape = 1;
  break;
 case 3:
  if(towardsNegative) {
   --x;
   ++y;
   checkBelow = false;
  } else {
   ++x;
  }
  shape = 1;
  break;
 case 4:
  if(towardsNegative) {
   ++z;
  } else {
   --z;
   ++y;
   checkBelow = false;
  }
  shape = 0;
  break;
 case 5:
  if(towardsNegative) {
   ++z;
   ++y;
   checkBelow = false;
  } else {
   --z;
  }
  shape = 0;
  break;
 default:
  break;
 }
 if(isPoweredByRail(world, x, y, z, towardsNegative, depth, shape)) {
  return true;
 }
 return checkBelow && isPoweredByRail(world, x, y - 1, z, towardsNegative, depth, shape);
}
bool RailBlock::isPoweredByRail(World* world, int x, int y, int z, bool towardsNegative, int depth, int shape) {
 if(world == nullptr || Block::POWERED_RAIL == nullptr) {
  return false;
 }
 const int blockId = world->getBlockId(x, y, z);
 if(blockId != Block::POWERED_RAIL->id) {
  return false;
 }
 int meta = world->getBlockMeta(x, y, z);
 const int railShape = meta & 7;
 if(shape == 1 && (railShape == 0 || railShape == 4 || railShape == 5)) {
  return false;
 }
 if(shape == 0 && (railShape == 1 || railShape == 2 || railShape == 3)) {
  return false;
 }
 if((meta & 8) != 0) {
  if(world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z)) {
   return true;
  }
  return isPoweredByConnectedRails(world, x, y, z, meta, towardsNegative, depth + 1);
 }
 return false;
}
void RailBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
 setBoundingBox(getRenderBounds(blockView, x, y, z));
}
net::minecraft::Box RailBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
 const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
 if(meta >= 2 && meta <= 5) {
  return {0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f};
 }
 return {0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f};
}
std::optional<net::minecraft::HitResult> RailBlock::raycast(
    World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const {
 if(world != nullptr) {
  const_cast<RailBlock*>(this)->updateBoundingBox(world, x, y, z);
 }
 return Block::raycast(world, x, y, z, startPos, endPos);
}
void RailBlock::registerClass() {
 Block::POWERED_RAIL = (new RailBlock(kBlockId, 179, true))
                           ->setHardness(0.7f)
                           ->setSoundGroup(&kMetalSound)
                           ->setTranslationKey("goldenRail")
                           ->ignoreMetaUpdates();
 Block::RAIL = (new RailBlock(66, 128, false))
                   ->setHardness(0.7f)
                   ->setSoundGroup(&kMetalSound)
                   ->setTranslationKey("rail")
                   ->ignoreMetaUpdates();
}
void RailBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(
     ItemStack(Block::RAIL, 16),
     {std::string("X X"), std::string("X#X"), std::string("X X"), 'X', Item::byRawId(9), '#', Item::byRawId(24)});
 recipeManager.addShapedRecipe(ItemStack(Block::POWERED_RAIL, 6),
                               {std::string("X X"),
                                std::string("X#X"),
                                std::string("XRX"),
                                'X',
                                Item::byRawId(10),
                                'R',
                                Item::byRawId(75),
                                '#',
                                Item::byRawId(24)});
}
namespace {
} // namespace
MC_REGISTER_BLOCK(RailBlock)
} // namespace net::minecraft::block
