#include "net/minecraft/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include "net/minecraft/block/CactusBlock.hpp"
#include "net/minecraft/block/CakeBlock.hpp"
#include "net/minecraft/block/CobwebBlock.hpp"
#include "net/minecraft/block/CropBlock.hpp"
#include "net/minecraft/block/DeadBushBlock.hpp"
#include "net/minecraft/block/DirtBlock.hpp"
#include "net/minecraft/block/FarmlandBlock.hpp"
#include "net/minecraft/block/GravelBlock.hpp"
#include "net/minecraft/block/MushroomPlantBlock.hpp"
#include "net/minecraft/block/ObsidianBlock.hpp"
#include "net/minecraft/block/OreBlock.hpp"
#include "net/minecraft/block/OreStorageBlock.hpp"
#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/block/SandBlock.hpp"
#include "net/minecraft/block/SoulSandBlock.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"
#include "net/minecraft/block/SpongeBlock.hpp"
#include "net/minecraft/block/SugarCaneBlock.hpp"
#include "net/minecraft/block/TallPlantBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
// Out-of-line bodies for Block. Anything that dereferences World / ItemStack /
// Entity lives here so Block.hpp stays free of the cycle-forming includes.
namespace {
net::minecraft::BlockSoundGroup kGravelSound("gravel", 1.0f, 1.0f);
net::minecraft::BlockSoundGroup kMetalSound("stone", 1.0f, 1.5f);
net::minecraft::BlockSoundGroup kDirtSound("grass", 1.0f, 1.0f);
net::minecraft::BlockSoundGroup kClothSound("cloth", 1.0f, 1.0f);
} // namespace
namespace net::minecraft::block {
net::minecraft::BlockSoundGroup Block::DEFAULT_SOUND_GROUP("stone", 1.0f, 1.0f);
net::minecraft::BlockSoundGroup Block::GLASS_BLOCK_SOUNDS("stone", 1.0f, 1.0f, "random.glass");
net::minecraft::BlockSoundGroup Block::SAND_BLOCK_SOUNDS("sand", 1.0f, 1.0f, "step.gravel");
// --- static storage (Java's parallel arrays) ---
std::array<Block*, Block::BLOCK_COUNT> Block::BLOCKS{};
std::array<bool, Block::BLOCK_COUNT> Block::BLOCKS_RANDOM_TICK{};
std::array<bool, Block::BLOCK_COUNT> Block::BLOCKS_OPAQUE{};
std::array<bool, Block::BLOCK_COUNT> Block::BLOCKS_WITH_ENTITY{};
std::array<int, Block::BLOCK_COUNT> Block::BLOCKS_LIGHT_OPACITY{};
std::array<bool, Block::BLOCK_COUNT> Block::BLOCKS_ALLOW_VISION{};
std::array<bool, Block::BLOCK_COUNT> Block::BLOCKS_IGNORE_META_UPDATE{};
namespace {
std::array<bool, Block::BLOCK_COUNT> g_explicitLightOpacity{};
}
// --- named static instances (assigned in BlocksRegistration.cpp) ---
Block* Block::STONE = nullptr;
Block* Block::GRASS_BLOCK = nullptr;
Block* Block::DIRT = nullptr;
Block* Block::COBBLESTONE = nullptr;
Block* Block::PLANKS = nullptr;
Block* Block::SAPLING = nullptr;
Block* Block::BEDROCK = nullptr;
Block* Block::FLOWING_WATER = nullptr;
Block* Block::WATER = nullptr;
Block* Block::FLOWING_LAVA = nullptr;
Block* Block::LAVA = nullptr;
Block* Block::SAND = nullptr;
Block* Block::GRAVEL = nullptr;
Block* Block::GOLD_ORE = nullptr;
Block* Block::IRON_ORE = nullptr;
Block* Block::COAL_ORE = nullptr;
Block* Block::LOG = nullptr;
Block* Block::LEAVES = nullptr;
Block* Block::SPONGE = nullptr;
Block* Block::GLASS = nullptr;
Block* Block::LAPIS_ORE = nullptr;
Block* Block::LAPIS_BLOCK = nullptr;
Block* Block::DISPENSER = nullptr;
Block* Block::SANDSTONE = nullptr;
Block* Block::NOTE_BLOCK = nullptr;
Block* Block::BED = nullptr;
Block* Block::POWERED_RAIL = nullptr;
Block* Block::DETECTOR_RAIL = nullptr;
Block* Block::STICKY_PISTON = nullptr;
Block* Block::COBWEB = nullptr;
Block* Block::GRASS = nullptr;
Block* Block::DEAD_BUSH = nullptr;
Block* Block::PISTON = nullptr;
Block* Block::PISTON_HEAD = nullptr;
Block* Block::WOOL = nullptr;
Block* Block::MOVING_PISTON = nullptr;
Block* Block::DANDELION = nullptr;
Block* Block::ROSE = nullptr;
Block* Block::BROWN_MUSHROOM = nullptr;
Block* Block::RED_MUSHROOM = nullptr;
Block* Block::GOLD_BLOCK = nullptr;
Block* Block::IRON_BLOCK = nullptr;
Block* Block::DOUBLE_SLAB = nullptr;
Block* Block::SLAB = nullptr;
Block* Block::BRICKS = nullptr;
Block* Block::TNT = nullptr;
Block* Block::BOOKSHELF = nullptr;
Block* Block::MOSSY_COBBLESTONE = nullptr;
Block* Block::OBSIDIAN = nullptr;
Block* Block::TORCH = nullptr;
Block* Block::FIRE = nullptr;
Block* Block::SPAWNER = nullptr;
Block* Block::WOODEN_STAIRS = nullptr;
Block* Block::CHEST = nullptr;
Block* Block::REDSTONE_WIRE = nullptr;
Block* Block::DIAMOND_ORE = nullptr;
Block* Block::DIAMOND_BLOCK = nullptr;
Block* Block::CRAFTING_TABLE = nullptr;
Block* Block::WHEAT = nullptr;
Block* Block::FARMLAND = nullptr;
Block* Block::FURNACE = nullptr;
Block* Block::LIT_FURNACE = nullptr;
Block* Block::SIGN = nullptr;
Block* Block::DOOR = nullptr;
Block* Block::LADDER = nullptr;
Block* Block::RAIL = nullptr;
Block* Block::COBBLESTONE_STAIRS = nullptr;
Block* Block::WALL_SIGN = nullptr;
Block* Block::LEVER = nullptr;
Block* Block::STONE_PRESSURE_PLATE = nullptr;
Block* Block::IRON_DOOR = nullptr;
Block* Block::WOODEN_PRESSURE_PLATE = nullptr;
Block* Block::REDSTONE_ORE = nullptr;
Block* Block::LIT_REDSTONE_ORE = nullptr;
Block* Block::REDSTONE_TORCH = nullptr;
Block* Block::LIT_REDSTONE_TORCH = nullptr;
Block* Block::BUTTON = nullptr;
Block* Block::SNOW = nullptr;
Block* Block::ICE = nullptr;
Block* Block::SNOW_BLOCK = nullptr;
Block* Block::CACTUS = nullptr;
Block* Block::CLAY = nullptr;
Block* Block::SUGAR_CANE = nullptr;
Block* Block::JUKEBOX = nullptr;
Block* Block::FENCE = nullptr;
Block* Block::PUMPKIN = nullptr;
Block* Block::NETHERRACK = nullptr;
Block* Block::SOUL_SAND = nullptr;
Block* Block::GLOWSTONE = nullptr;
Block* Block::NETHER_PORTAL = nullptr;
Block* Block::JACK_O_LANTERN = nullptr;
Block* Block::CAKE = nullptr;
Block* Block::REPEATER = nullptr;
Block* Block::POWERED_REPEATER = nullptr;
Block* Block::LOCKED_CHEST = nullptr;
Block* Block::TRAPDOOR = nullptr;
std::string Block::getTranslatedName() const {
 return net::minecraft::client::resource::language::I18n::getTranslation(getTranslationKey() + ".name");
}
// --- constructors ---
Block::Block(int id, Material& material) : id(id), material(material) {
 soundGroup = &DEFAULT_SOUND_GROUP;
 BLOCKS[static_cast<std::size_t>(id)] = this;
 setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
 BLOCKS_OPAQUE[static_cast<std::size_t>(id)] = isOpaque();
 BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)] = isOpaque() ? 255 : 0;
 BLOCKS_ALLOW_VISION[static_cast<std::size_t>(id)] = !material.blocksVision();
 BLOCKS_WITH_ENTITY[static_cast<std::size_t>(id)] = false;
}
Block::Block(int id, int textureId, Material& material) : Block(id, material) {
 this->textureId = textureId;
}
// --- setters ---
Block* Block::setOpacity(int i) {
 BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)] = i;
 g_explicitLightOpacity[static_cast<std::size_t>(id)] = true;
 return this;
}
Block* Block::setLuminance(float fractionalValue) {
 world::light::UnifiedLightRegistry::setBlockEmission(id, static_cast<int>(15.0f * fractionalValue));
 return this;
}
int Block::emission() const noexcept {
 return world::light::UnifiedLightRegistry::blockEmission(id);
}
Block* Block::setLightColor(float red, float green, float blue) {
 world::light::UnifiedLightRegistry::setBlockLightColor(id, red, green, blue);
 return this;
}
Block* Block::setResistance(float resistance) {
 resistance_ = resistance * 3.0f;
 return this;
}
Block* Block::setHardness(float hardness) {
 hardness_ = hardness;
 if(resistance_ < hardness * 5.0f) {
  resistance_ = hardness * 5.0f;
 }
 return this;
}
Block* Block::setUnbreakable() {
 setHardness(-1.0f);
 return this;
}
Block* Block::setTickRandomly(bool value) {
 BLOCKS_RANDOM_TICK[static_cast<std::size_t>(id)] = value;
 return this;
}
Block* Block::ignoreMetaUpdates() {
 BLOCKS_IGNORE_META_UPDATE[static_cast<std::size_t>(id)] = true;
 return this;
}
Block* Block::disableTrackingStatistics() {
 shouldTrackStatistics_ = false;
 return this;
}
Block* Block::setTranslationKey(const std::string& name) {
 translationKey_ = "tile." + name;
 return this;
}
void Block::setBoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) {
 this->minX = minX;
 this->minY = minY;
 this->minZ = minZ;
 this->maxX = maxX;
 this->maxY = maxY;
 this->maxZ = maxZ;
}
// --- bounding boxes ---
Box Block::getBoundingBox(World* /*world*/, int x, int y, int z) const {
 return {static_cast<double>(x) + minX,
         static_cast<double>(y) + minY,
         static_cast<double>(z) + minZ,
         static_cast<double>(x) + maxX,
         static_cast<double>(y) + maxY,
         static_cast<double>(z) + maxZ};
}
std::optional<net::minecraft::Box> Block::getCollisionShape(World* /*world*/, int x, int y, int z) const {
 return net::minecraft::Box{static_cast<double>(x) + minX,
                            static_cast<double>(y) + minY,
                            static_cast<double>(z) + minZ,
                            static_cast<double>(x) + maxX,
                            static_cast<double>(y) + maxY,
                            static_cast<double>(z) + maxZ};
}
void Block::addIntersectingBoundingBox(
    World* world, int x, int y, int z, const Box& box, std::vector<Box>& boxes) const {
 const std::optional<net::minecraft::Box> collisionShape = getCollisionShape(world, x, y, z);
 if(collisionShape.has_value() && box.intersects(*collisionShape)) {
  boxes.push_back(*collisionShape);
 }
}
// --- placement ---
bool Block::canPlaceAt(World* world, int x, int y, int z) const {
 int n = world->getBlockId(x, y, z);
 if(n == 0) {
  return true;
 }
 Block* other = BLOCKS[static_cast<std::size_t>(n)];
 return other != nullptr && other->material.isReplaceable();
}
bool Block::canPlaceAt(World* world, int x, int y, int z, int /*side*/) const {
 return canPlaceAt(world, x, y, z);
}
// --- drops ---
void Block::dropStacks(World* world, int x, int y, int z, int meta) {
 dropStacks(world, x, y, z, meta, 1.0f);
}
void Block::dropStacks(World* world, int x, int y, int z, int meta, float luck) {
 if(world->isRemote()) {
  return;
 }
 int n = getDroppedItemCount(world->random());
 for(int i = 0; i < n; ++i) {
  if(world->random().nextFloat() > luck) {
   continue;
  }
  int itemId = getDroppedItemId(meta, world->random());
  if(itemId <= 0) {
   continue;
  }
  dropStack(world, x, y, z, ItemStack(itemId, 1, getDroppedItemMeta(meta)));
 }
}
void Block::dropStack(World* world, int x, int y, int z, const net::minecraft::ItemStack& stack) {
 if(world->isRemote() || stack.empty()) {
  return;
 }
 const float spread = 0.7f;
 const double offsetX =
     static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
 const double offsetY =
     static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
 const double offsetZ =
     static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
 auto* itemEntity = new ItemEntity(world,
                                   static_cast<double>(x) + offsetX,
                                   static_cast<double>(y) + offsetY,
                                   static_cast<double>(z) + offsetZ,
                                   stack);
 itemEntity->pickupDelay = 10;
 world->spawnEntity(itemEntity);
}
void Block::afterBreak(World* world, net::minecraft::PlayerEntity* player, int x, int y, int z, int meta) {
 if(player != nullptr && id >= 0 && id < BLOCK_COUNT && BLOCKS[static_cast<std::size_t>(id)] != nullptr &&
    BLOCKS[static_cast<std::size_t>(id)]->isTrackingStatistics()) {
  player->increaseStat(stat::Stats::mineBlockStatId(id), 1);
 }
 dropStacks(world, x, y, z, meta);
}
float Block::getHardness(net::minecraft::PlayerEntity* player) const {
 if(hardness_ < 0.0f) {
  return 0.0f;
 }
 if(player == nullptr) {
  return 1.0f / hardness_ / 100.0f;
 }
 if(!player->canHarvest(id)) {
  return 1.0f / hardness_ / 100.0f;
 }
 return player->getBlockBreakingSpeed(id) / hardness_ / 30.0f;
}
float Block::getBlastResistance(net::minecraft::Entity* /*entity*/) const {
 return resistance_ / 5.0f;
}
// --- raycast (faithful plane-intersection) ---
bool Block::containsInYZPlane(const std::optional<Vec3d>& v) const {
 if(!v) {
  return false;
 }
 return v->y >= minY && v->y <= maxY && v->z >= minZ && v->z <= maxZ;
}
bool Block::containsInXZPlane(const std::optional<Vec3d>& v) const {
 if(!v) {
  return false;
 }
 return v->x >= minX && v->x <= maxX && v->z >= minZ && v->z <= maxZ;
}
bool Block::containsInXYPlane(const std::optional<Vec3d>& v) const {
 if(!v) {
  return false;
 }
 return v->x >= minX && v->x <= maxX && v->y >= minY && v->y <= maxY;
}
namespace {
// Linear interpolation of a segment to a target plane, faithful to Vec3d's
// interpolateByX/Y/Z (returns nullopt when the segment does not cross the plane).
std::optional<Vec3d> interpByX(const net::minecraft::Vec3d& a, const net::minecraft::Vec3d& b, double x) {
 double dx = b.x - a.x;
 if(dx * dx < 1.0e-7) {
  return std::nullopt;
 }
 double t = (x - a.x) / dx;
 if(t < 0.0 || t > 1.0) {
  return std::nullopt;
 }
 return Vec3d{a.x + dx * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
std::optional<Vec3d> interpByY(const net::minecraft::Vec3d& a, const net::minecraft::Vec3d& b, double y) {
 double dy = b.y - a.y;
 if(dy * dy < 1.0e-7) {
  return std::nullopt;
 }
 double t = (y - a.y) / dy;
 if(t < 0.0 || t > 1.0) {
  return std::nullopt;
 }
 return Vec3d{a.x + (b.x - a.x) * t, a.y + dy * t, a.z + (b.z - a.z) * t};
}
std::optional<Vec3d> interpByZ(const net::minecraft::Vec3d& a, const net::minecraft::Vec3d& b, double z) {
 double dz = b.z - a.z;
 if(dz * dz < 1.0e-7) {
  return std::nullopt;
 }
 double t = (z - a.z) / dz;
 if(t < 0.0 || t > 1.0) {
  return std::nullopt;
 }
 return Vec3d{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + dz * t};
}
double distSq(const net::minecraft::Vec3d& a, const net::minecraft::Vec3d& b) {
 double dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
 return dx * dx + dy * dy + dz * dz;
}
} // namespace
namespace {
bool containsInYzPlane(const std::optional<Vec3d>& v, double minY, double maxY, double minZ, double maxZ) {
 if(!v) {
  return false;
 }
 return v->y >= minY && v->y <= maxY && v->z >= minZ && v->z <= maxZ;
}
bool containsInXzPlane(const std::optional<Vec3d>& v, double minX, double maxX, double minZ, double maxZ) {
 if(!v) {
  return false;
 }
 return v->x >= minX && v->x <= maxX && v->z >= minZ && v->z <= maxZ;
}
bool containsInXyPlane(const std::optional<Vec3d>& v, double minX, double maxX, double minY, double maxY) {
 if(!v) {
  return false;
 }
 return v->x >= minX && v->x <= maxX && v->y >= minY && v->y <= maxY;
}
} // namespace
std::optional<net::minecraft::HitResult> Block::raycastLocalBounds(double localMinX,
                                                                   double localMinY,
                                                                   double localMinZ,
                                                                   double localMaxX,
                                                                   double localMaxY,
                                                                   double localMaxZ,
                                                                   int x,
                                                                   int y,
                                                                   int z,
                                                                   Vec3d startPos,
                                                                   Vec3d endPos) {
 startPos = Vec3d{startPos.x - x, startPos.y - y, startPos.z - z};
 endPos = Vec3d{endPos.x - x, endPos.y - y, endPos.z - z};
 std::optional<Vec3d> v1 = interpByX(startPos, endPos, localMinX);
 std::optional<Vec3d> v2 = interpByX(startPos, endPos, localMaxX);
 std::optional<Vec3d> v3 = interpByY(startPos, endPos, localMinY);
 std::optional<Vec3d> v4 = interpByY(startPos, endPos, localMaxY);
 std::optional<Vec3d> v5 = interpByZ(startPos, endPos, localMinZ);
 std::optional<Vec3d> v6 = interpByZ(startPos, endPos, localMaxZ);
 if(!containsInYzPlane(v1, localMinY, localMaxY, localMinZ, localMaxZ)) {
  v1.reset();
 }
 if(!containsInYzPlane(v2, localMinY, localMaxY, localMinZ, localMaxZ)) {
  v2.reset();
 }
 if(!containsInXzPlane(v3, localMinX, localMaxX, localMinZ, localMaxZ)) {
  v3.reset();
 }
 if(!containsInXzPlane(v4, localMinX, localMaxX, localMinZ, localMaxZ)) {
  v4.reset();
 }
 if(!containsInXyPlane(v5, localMinX, localMaxX, localMinY, localMaxY)) {
  v5.reset();
 }
 if(!containsInXyPlane(v6, localMinX, localMaxX, localMinY, localMaxY)) {
  v6.reset();
 }
 const std::optional<Vec3d>* candidates[6] = {&v1, &v2, &v3, &v4, &v5, &v6};
 int bestIndex = 0;
 for(int i = 0; i < 6; ++i) {
  if(!candidates[i]->has_value()) {
   continue;
  }
  if(bestIndex == 0 || distSq(startPos, **candidates[i]) < distSq(startPos, **candidates[bestIndex - 1])) {
   bestIndex = i + 1;
  }
 }
 if(bestIndex == 0) {
  return std::nullopt;
 }
 static constexpr int kSideForIndex[6] = {4, 5, 0, 1, 2, 3};
 const net::minecraft::Vec3d& bestVec = **candidates[bestIndex - 1];
 const int side = kSideForIndex[bestIndex - 1];
 const Vec3d hit{bestVec.x + x, bestVec.y + y, bestVec.z + z};
 return HitResult{x, y, z, side, hit};
}
std::optional<net::minecraft::HitResult> Block::raycast(
    World* world, int x, int y, int z, Vec3d startPos, Vec3d endPos) const {
 const_cast<Block*>(this)->updateBoundingBox(world, x, y, z);
 return raycastLocalBounds(minX, minY, minZ, maxX, maxY, maxZ, x, y, z, startPos, endPos);
}
void Block::registerBlockItem() {
 if(Item::ITEMS[static_cast<std::size_t>(id)] != nullptr) {
  return;
 }
 Item::ITEMS[static_cast<std::size_t>(id)] = new item::BlockItem(id - 256);
 init();
}
void finalizeBlockRegistryProperties() {
 for(std::size_t i = 0; i < Block::BLOCKS.size(); ++i) {
  Block* block = Block::BLOCKS[i];
  if(block == nullptr) {
   continue;
  }
  // Java sets static render/collision bounds in constructors or
  // setupRenderBoundingBox(); inventory-only setup in beta still has to run
  // once so default getRenderBounds() and getCollisionShapeLocal() match.
  block->setupRenderBoundingBox();
  const bool opaque = block->isOpaque();
  Block::BLOCKS_OPAQUE[i] = opaque;
  if(!g_explicitLightOpacity[i]) {
   Block::BLOCKS_LIGHT_OPACITY[i] = opaque ? 255 : 0;
  }
  Block::BLOCKS_ALLOW_VISION[i] = !block->material.blocksVision();
 }
 Block::BLOCKS_ALLOW_VISION[0] = true;
}
void initializeBlocks() {
 registry::Registry::bootstrap();
}
bool Block::usesNeighborLightSampling(int blockId) {
 if(blockId <= 0 || blockId >= BLOCK_COUNT) {
  return false;
 }
 return (SLAB != nullptr && blockId == SLAB->id) || (FARMLAND != nullptr && blockId == FARMLAND->id) ||
        (WOODEN_STAIRS != nullptr && blockId == WOODEN_STAIRS->id) ||
        (COBBLESTONE_STAIRS != nullptr && blockId == COBBLESTONE_STAIRS->id) ||
        (TRAPDOOR != nullptr && blockId == TRAPDOOR->id);
}
float Block::getLuminance(const BlockView* blockView, int x, int y, int z) const {
 if(blockView == nullptr) {
  return 1.0f;
 }
 return blockView->getNaturalBrightness(x, y, z, emission());
}
bool Block::isSideVisibleForBounds(const BlockView* blockView, int x, int y, int z, int side, const Box& bounds) const {
 if(side == 0 && bounds.minY > 0.0) {
  return true;
 }
 if(side == 1 && bounds.maxY < 1.0) {
  return true;
 }
 if(side == 2 && bounds.minZ > 0.0) {
  return true;
 }
 if(side == 3 && bounds.maxZ < 1.0) {
  return true;
 }
 if(side == 4 && bounds.minX > 0.0) {
  return true;
 }
 if(side == 5 && bounds.maxX < 1.0) {
  return true;
 }
 if(blockView == nullptr) {
  return true;
 }
 return !blockView->isBlockOpaqueCube(x, y, z);
}
bool Block::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const {
 return isSideVisibleForBounds(blockView, x, y, z, side, getCollisionShapeLocal());
}
int Block::getTextureId(const BlockView* blockView, int x, int y, int z, int side) const {
 if(blockView == nullptr) {
  return getTexture(side);
 }
 return getTexture(side, blockView->getBlockMeta(x, y, z));
}
void Block::updateBoundingBox(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/) {
}
int Block::getColorMultiplier(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/) const {
 return 0xFFFFFF;
}
void Block::setupRenderBoundingBox() {
}
int Block::textureForSide(int side, int sides, int bottom, int top, int frontSide, int front) {
 if(side == FACE_BOTTOM && bottom >= 0) {
  return bottom;
 }
 if(side == FACE_TOP && top >= 0) {
  return top;
 }
 if(frontSide >= 0 && side == frontSide && front >= 0) {
  return front;
 }
 return sides;
}
TerrainAtlasUv Block::terrainTileUv(int textureId) {
 const int texU = (textureId & 0xF) << 4;
 const int texV = textureId & 0xF0;
 return {
     static_cast<double>(texU) / 256.0,
     static_cast<double>(texU + 16) / 256.0,
     static_cast<double>(texV) / 256.0,
     static_cast<double>(texV + 16) / 256.0,
 };
}
TerrainAtlasUv Block::terrainStripUv(int textureId, double scrollU, double stripHeight) {
 const int texU = (textureId & 0xF) << 4;
 const int texV = textureId & 0xF0;
 return {
     static_cast<double>(texU) / 256.0,
     (static_cast<double>(texU) + scrollU) / 256.0,
     static_cast<double>(texV) / 256.0,
     (static_cast<double>(texV) + stripHeight) / 256.0,
 };
}
} // namespace net::minecraft::block
// Vanilla blocks simple enough to not warrant their own translation unit
// (plain constructor + setter chain, no recipes, no custom block items),
// plus the registry finalize and default-block-item passes. Each addBlock
// keeps the original registering id so bootstrap order is unchanged.
namespace net::minecraft::block {
namespace {
namespace mat = material;
struct VanillaBlocksRegistrar {
 VanillaBlocksRegistrar() {
  using registry::Registry;
  Registry::addBlock(3, [] {
   Block::DIRT =
       (new DirtBlock(3, 2))->setHardness(0.5f)->setSoundGroup(&kGravelSound)->setTranslationKey("dirt");
  });
  Registry::addBlock(4, [] {
   Block::COBBLESTONE = (new Block(4, 16, mat::Material::STONE))
                            ->setHardness(2.0f)
                            ->setResistance(10.0f)
                            ->setTranslationKey("stonebrick");
  });
  Registry::addBlock(7, [] {
   Block::BEDROCK = (new Block(7, 17, mat::Material::STONE))
                        ->setUnbreakable()
                        ->setResistance(6000000.0f)
                        ->setTranslationKey("bedrock")
                        ->disableTrackingStatistics();
  });
  Registry::addBlock(12, [] {
   Block::SAND = (new SandBlock(12, 18))
                     ->setHardness(0.5f)
                     ->setSoundGroup(&Block::SAND_BLOCK_SOUNDS)
                     ->setTranslationKey("sand");
  });
  Registry::addBlock(13, [] {
   Block::GRAVEL =
       (new GravelBlock(13, 19))->setHardness(0.6f)->setSoundGroup(&kGravelSound)->setTranslationKey("gravel");
  });
  Registry::addBlock(14, [] {
   Block::GOLD_ORE =
       (new OreBlock(14, 32))->setHardness(3.0f)->setResistance(5.0f)->setTranslationKey("oreGold");
   Block::IRON_ORE =
       (new OreBlock(15, 33))->setHardness(3.0f)->setResistance(5.0f)->setTranslationKey("oreIron");
   Block::COAL_ORE =
       (new OreBlock(16, 34))->setHardness(3.0f)->setResistance(5.0f)->setTranslationKey("oreCoal");
   Block::LAPIS_ORE =
       (new OreBlock(21, 160))->setHardness(3.0f)->setResistance(5.0f)->setTranslationKey("oreLapis");
   Block::DIAMOND_ORE =
       (new OreBlock(56, 50))->setHardness(3.0f)->setResistance(5.0f)->setTranslationKey("oreDiamond");
  });
  Registry::addBlock(19, [] {
   Block::SPONGE =
       (new SpongeBlock(19))->setHardness(0.6f)->setSoundGroup(&kDirtSound)->setTranslationKey("sponge");
  });
  Registry::addBlock(22, [] {
   Block::LAPIS_BLOCK = (new Block(22, 144, mat::Material::STONE))
                            ->setHardness(3.0f)
                            ->setResistance(5.0f)
                            ->setTranslationKey("blockLapis");
  });
  Registry::addBlock(30, [] {
   Block::COBWEB = (new CobwebBlock(30, 11))->setOpacity(1)->setHardness(4.0f)->setTranslationKey("web");
  });
  Registry::addBlock(31, [] {
   Block::GRASS = (new TallPlantBlock(31, 39))
                      ->setHardness(0.0f)
                      ->setSoundGroup(&kDirtSound)
                      ->setTranslationKey("tallgrass");
  });
  Registry::addBlock(32, [] {
   Block::DEAD_BUSH = (new DeadBushBlock(32, 55))
                          ->setHardness(0.0f)
                          ->setSoundGroup(&kDirtSound)
                          ->setTranslationKey("deadbush");
  });
  Registry::addBlock(37, [] {
   Block::DANDELION =
       (new PlantBlock(37, 13))->setHardness(0.0f)->setSoundGroup(&kDirtSound)->setTranslationKey("flower");
   Block::ROSE =
       (new PlantBlock(38, 12))->setHardness(0.0f)->setSoundGroup(&kDirtSound)->setTranslationKey("rose");
  });
  Registry::addBlock(39, [] {
   Block::BROWN_MUSHROOM = (new MushroomPlantBlock(39, 29))
                               ->setHardness(0.0f)
                               ->setSoundGroup(&kDirtSound)
                               ->setLuminance(0.125f)
                               ->setTranslationKey("mushroom");
   Block::RED_MUSHROOM = (new MushroomPlantBlock(40, 28))
                             ->setHardness(0.0f)
                             ->setSoundGroup(&kDirtSound)
                             ->setTranslationKey("mushroom");
  });
  Registry::addBlock(41, [] {
   Block::GOLD_BLOCK = (new OreStorageBlock(41, 23))
                           ->setHardness(3.0f)
                           ->setResistance(10.0f)
                           ->setSoundGroup(&kMetalSound)
                           ->setTranslationKey("blockGold");
   Block::IRON_BLOCK = (new OreStorageBlock(42, 22))
                           ->setHardness(5.0f)
                           ->setResistance(10.0f)
                           ->setSoundGroup(&kMetalSound)
                           ->setTranslationKey("blockIron");
   Block::DIAMOND_BLOCK = (new OreStorageBlock(57, 24))
                              ->setHardness(5.0f)
                              ->setResistance(10.0f)
                              ->setSoundGroup(&kMetalSound)
                              ->setTranslationKey("blockDiamond");
  });
  Registry::addBlock(48, [] {
   Block::MOSSY_COBBLESTONE = (new Block(48, 36, mat::Material::STONE))
                                  ->setHardness(2.0f)
                                  ->setResistance(10.0f)
                                  ->setTranslationKey("stoneMoss");
  });
  Registry::addBlock(49, [] {
   Block::OBSIDIAN =
       (new ObsidianBlock(49, 37))->setHardness(10.0f)->setResistance(2000.0f)->setTranslationKey("obsidian");
  });
  Registry::addBlock(52, [] {
   Block::SPAWNER = (new SpawnerBlock(52, 65))
                        ->setHardness(5.0f)
                        ->setSoundGroup(&kMetalSound)
                        ->setTranslationKey("mobSpawner")
                        ->disableTrackingStatistics();
  });
  Registry::addBlock(59, [] {
   Block::WHEAT = (new CropBlock(59, 88))
                      ->setHardness(0.0f)
                      ->setSoundGroup(&kDirtSound)
                      ->setTranslationKey("crops")
                      ->disableTrackingStatistics()
                      ->ignoreMetaUpdates();
  });
  Registry::addBlock(60, [] {
   Block::FARMLAND =
       (new FarmlandBlock(60))->setHardness(0.6f)->setSoundGroup(&kGravelSound)->setTranslationKey("farmland");
  });
  Registry::addBlock(81, [] {
   Block::CACTUS =
       (new CactusBlock(81, 70))->setHardness(0.4f)->setSoundGroup(&kClothSound)->setTranslationKey("cactus");
  });
  Registry::addBlock(83, [] {
   Block::SUGAR_CANE = (new SugarCaneBlock(83, 73))
                           ->setHardness(0.0f)
                           ->setSoundGroup(&kDirtSound)
                           ->setTranslationKey("reeds")
                           ->disableTrackingStatistics();
  });
  Registry::addBlock(87, [] {
   Block::NETHERRACK =
       (new Block(87, 103, mat::Material::STONE))->setHardness(0.4f)->setTranslationKey("hellrock");
  });
  Registry::addBlock(88, [] {
   Block::SOUL_SAND = (new SoulSandBlock(88, 104))
                          ->setHardness(0.5f)
                          ->setSoundGroup(&Block::SAND_BLOCK_SOUNDS)
                          ->setTranslationKey("hellsand");
  });
  Registry::addBlock(92, [] {
   Block::CAKE = (new CakeBlock(92, 121))
                     ->setHardness(0.5f)
                     ->setSoundGroup(&kClothSound)
                     ->setTranslationKey("cake")
                     ->disableTrackingStatistics()
                     ->ignoreMetaUpdates();
  });
 }
};
// Folded here from the former BlockRegistryFinalizer.cpp: recomputes opacity /
// vision tables once every block instance + setter chain has run (C++ can't
// virtual-dispatch from a base constructor).
struct BlockRegistryFinalizePass {
 static void registerClass() {
  finalizeBlockRegistryProperties();
 }
};
// Folded here from the former GenericBlockItemRegistrar.cpp: the single
// default-block-item path. Every registered block that didn't register a custom
// item gets a default BlockItem + init() (the guard in registerBlockItem()
// no-ops blocks that already have one). Runs after explicit block-items.
struct GenericBlockItemPass {
 static void registerClass() {
  for(int blockId = 0; blockId < 256; ++blockId) {
   Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
   if(block == nullptr) {
    continue;
   }
   block->registerBlockItem();
  }
 }
};
const VanillaBlocksRegistrar s_vanillaBlocks;
static registry::RegisterPhase<BlockRegistryFinalizePass> s_finalize(mod::LifecyclePhase::Init, 59000);
static registry::RegisterPhase<GenericBlockItemPass> s_genericBlockItems(mod::LifecyclePhase::Init,
                                                                         registry::kGenericBlockItemOrder);
}
} // namespace net::minecraft::block
