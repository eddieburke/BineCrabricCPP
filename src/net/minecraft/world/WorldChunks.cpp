#include <sstream>
#include <stdexcept>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft {
bool World::isRegionLoaded(int x, int y, int z, int range) const {
 return isRegionLoaded(x - range, y - range, z - range, x + range, y + range, z + range);
}
bool World::isRegionLoaded(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const {
 if(maxY < 0 || minY >= Chunk::height) {
  return false;
 }
 minX >>= 4;
 minY >>= 4;
 minZ >>= 4;
 maxX >>= 4;
 maxY >>= 4;
 maxZ >>= 4;
 for(int x = minX; x <= maxX; ++x) {
  for(int z = minZ; z <= maxZ; ++z) {
   if(getChunkIfLoaded(x << 4, z << 4) == nullptr) {
    return false;
   }
  }
 }
 return true;
}
int World::getLightLevel(int x, int y, int z) const {
 return getLightLevel(x, y, z, true);
}
int World::getLightLevelAbove(int x, int y, int z) const {
 return getLightLevel(x, y + 1, z);
}
bool World::hasEnoughLightToGrowPlant(int x, int y, int z) const {
 if(getBrightness(x, y, z) >= 8) {
  return true;
 }
 return hasSkyLight(x, y, z);
}
int World::getLightLevel(int x, int y, int z, bool useNeighborLight) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return 15;
 }
 if(useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
  int brightness = getLightLevel(x, y + 1, z, false);
  brightness = std::max(brightness, getLightLevel(x + 1, y, z, false));
  brightness = std::max(brightness, getLightLevel(x - 1, y, z, false));
  brightness = std::max(brightness, getLightLevel(x, y, z + 1, false));
  brightness = std::max(brightness, getLightLevel(x, y, z - 1, false));
  return brightness;
 }
 if(y < 0) {
  return 0;
 }
 if(y >= Chunk::height) {
  const int skyBrightness = 15 - ambientDarkness;
  return skyBrightness < 0 ? 0 : skyBrightness;
 }
 const Chunk& chunk = getChunk(chunk_coord(x), chunk_coord(z));
 return chunk.getLight(mod_16(x), y, mod_16(z), ambientDarkness);
}
float World::getNaturalBrightness(int x, int y, int z, int blockLight) const {
 int brightness = getLightLevel(x, y, z);
 if(brightness < blockLight) {
  brightness = blockLight;
 }
 if(dimension == nullptr) {
  return Dimension::luminanceForLightLevel(brightness);
 }
 return dimension->lightLevelToLuminance[static_cast<std::size_t>(brightness)];
}
float World::getLightBrightness(int x, int y, int z) const {
 if(dimension == nullptr) {
  return Dimension::luminanceForLightLevel(getLightLevel(x, y, z));
 }
 return dimension->lightLevelToLuminance[static_cast<std::size_t>(getLightLevel(x, y, z))];
}
Chunk& World::ensureChunk(int blockX, int blockZ) {
 if(chunkCache_ != nullptr) {
  return chunkCache_->getChunk(chunk_coord(blockX), chunk_coord(blockZ));
 }
 const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
 auto it = chunks_.find(pos);
 if(it == chunks_.end()) {
  auto [inserted, _] = chunks_.emplace(pos, chunkGenerator_.loadChunk(nullptr, pos.x, pos.z));
  it = inserted;
  registerChunkForLighting(&it->second);
 }
 return it->second;
}
const Chunk* World::getChunkIfLoaded(int blockX, int blockZ) const {
  if(chunkCache_ != nullptr) {
   const int chunkX = chunk_coord(blockX);
   const int chunkZ = chunk_coord(blockZ);
   return chunkCache_->isChunkLoaded(chunkX, chunkZ) ? &chunkCache_->getChunk(chunkX, chunkZ) : nullptr;
  }
  const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
  const auto it = chunks_.find(pos);
  if(it == chunks_.end()) {
   return nullptr;
  }
  return &it->second;
}
Chunk* World::getChunkIfLoaded(int blockX, int blockZ) {
  if(chunkCache_ != nullptr) {
   const int chunkX = chunk_coord(blockX);
   const int chunkZ = chunk_coord(blockZ);
   return chunkCache_->isChunkLoaded(chunkX, chunkZ) ? &chunkCache_->getChunk(chunkX, chunkZ) : nullptr;
  }
  const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
  const auto it = chunks_.find(pos);
  if(it == chunks_.end()) {
   return nullptr;
  }
  return &it->second;
}
const Chunk& World::getChunk(int chunkX, int chunkZ) const {
 if(chunkCache_ != nullptr) {
  return chunkCache_->getChunk(chunkX, chunkZ);
 }
 const auto it = chunks_.find(ChunkPos{chunkX, chunkZ});
 if(it == chunks_.end()) {
  throw std::runtime_error("Chunk not loaded");
 }
 return it->second;
}
Chunk& World::getChunk(int chunkX, int chunkZ) {
 if(chunkCache_ != nullptr) {
  return chunkCache_->getChunk(chunkX, chunkZ);
 }
 const ChunkPos pos{chunkX, chunkZ};
 auto it = chunks_.find(pos);
 if(it == chunks_.end()) {
  auto [inserted, _] = chunks_.emplace(pos, chunkGenerator_.loadChunk(nullptr, chunkX, chunkZ));
  it = inserted;
  registerChunkForLighting(&it->second);
 }
 return it->second;
}
int World::getBlockId(int x, int y, int z) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return 0;
 }
 if(y < 0 || y >= Chunk::height) {
  return 0;
 }
 return getChunk(chunk_coord(x), chunk_coord(z)).getBlockId(mod_16(x), y, mod_16(z));
}
int World::getBlockMeta(int x, int y, int z) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return 0;
 }
 if(y < 0 || y >= Chunk::height) {
  return 0;
 }
 return getChunk(chunk_coord(x), chunk_coord(z)).getBlockMeta(mod_16(x), y, mod_16(z));
}
bool World::hasChunk(int chunkX, int chunkZ) const {
  return getChunkIfLoaded(chunkX << 4, chunkZ << 4) != nullptr;
}
Chunk& World::getChunkFromPos(int x, int z) {
 return getChunk(chunk_coord(x), chunk_coord(z));
}
int World::getSpawnBlockId(int x, int z) const {
 int y = 63;
 while(getBlockId(x, y + 1, z) != 0) {
  ++y;
 }
 return getBlockId(x, y, z);
}
int World::getTopY(int x, int z) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return 0;
 }
 const Chunk* chunk = getChunkIfLoaded(x, z);
 if(chunk == nullptr) {
  return 0;
 }
 return chunk->getHeight(mod_16(x), mod_16(z));
}
const Biome& World::getBiome(int x, int z) const {
 if(BiomeSource* biomeSource = getBiomeSource(); biomeSource != nullptr) {
  return biomeSource->getBiome(x, z);
 }
 return Biome::getBiome(0.5, 0.5);
}
int World::getSpawnPositionValidityY(int x, int z) {
 Chunk& chunk = getChunkFromPos(x, z);
 const int localX = mod_16(x);
 const int localZ = mod_16(z);
 for(int y = 127; y > 0; --y) {
  const int id = chunk.getBlockId(localX, y, localZ);
  if(id == 0) {
   continue;
  }
  Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
  if(block == nullptr || !block->material.blocksMovement()) {
   continue;
  }
  return y + 1;
 }
 return -1;
}
int World::getBrightness(int x, int y, int z) const {
 return getBrightness(x, y, z, true);
}
int World::getBrightness(int x, int y, int z, bool useNeighborLight) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return 15;
 }
 if(useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
  int brightness = getBrightness(x, y + 1, z, false);
  brightness = std::max(brightness, getBrightness(x + 1, y, z, false));
  brightness = std::max(brightness, getBrightness(x - 1, y, z, false));
  brightness = std::max(brightness, getBrightness(x, y, z + 1, false));
  brightness = std::max(brightness, getBrightness(x, y, z - 1, false));
  return brightness;
 }
 if(y < 0) {
  return 0;
 }
 if(y >= Chunk::height) {
  y = Chunk::height - 1;
 }
 return getChunk(chunk_coord(x), chunk_coord(z)).getLight(mod_16(x), y, mod_16(z), 0);
}
int World::getBrightness(LightType type, int x, int y, int z) const {
 if(y < 0) {
  y = 0;
 }
 if(y >= Chunk::height) {
  y = Chunk::height - 1;
 }
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return lightValue(type);
 }
 if(getChunkIfLoaded(x, z) == nullptr) {
  return 0;
 }
 return getChunk(chunk_coord(x), chunk_coord(z)).getLight(type, mod_16(x), y, mod_16(z));
}
bool World::hasSkyLight(int x, int y, int z) const {
 return getChunk(chunk_coord(x), chunk_coord(z)).isAboveMaxHeight(mod_16(x), y, mod_16(z));
}
void World::loadSpawnChunks(int chunkRadius) {
 initializeBlocks();
 for(int cx = -chunkRadius; cx <= chunkRadius; ++cx) {
  for(int cz = -chunkRadius; cz <= chunkRadius; ++cz) {
   [[maybe_unused]] Chunk& chunk = getChunk(cx, cz);
  }
 }
}
std::size_t World::chunkCount() const noexcept {
 return chunks_.size();
}
const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& World::chunks() const noexcept {
 return chunks_;
}
std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& World::chunks() noexcept {
 return chunks_;
}
std::string World::describe() const {
 std::ostringstream out;
 out << "World[name=" << name_ << ", seed=" << seed_ << ", time=" << time_ << ", chunks=" << chunkCount() << "]";
 return out.str();
}
double World::getTemperature(int x, int z) const {
 if(BiomeSource* biomeSource = getBiomeSource(); biomeSource != nullptr) {
  // Java uses the raw sky-color temperature sampler here, not transformed biome climate.
  return biomeSource->getTemperature(x, z);
 }
 return 0.5;
}
double World::getDownfall(int x, int z) const {
 if(BiomeSource* biomeSource = getBiomeSource(); biomeSource != nullptr) {
  return biomeSource->sampleClimate(x, z).downfall;
 }
 return 0.5;
}
std::vector<std::uint8_t> World::getChunkData(int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(sizeX * sizeY * sizeZ * 5 / 2));
 const int chunkMinX = x >> 4;
 const int chunkMinZ = z >> 4;
 const int chunkMaxX = (x + sizeX - 1) >> 4;
 const int chunkMaxZ = (z + sizeZ - 1) >> 4;
 std::size_t offset = 0;
 int minY = y;
 int maxY = y + sizeY;
 if(minY < 0) {
  minY = 0;
 }
 if(maxY > Chunk::height) {
  maxY = Chunk::height;
 }
 for(int chunkX = chunkMinX; chunkX <= chunkMaxX; ++chunkX) {
  int minLocalX = x - chunkX * 16;
  int maxLocalX = x + sizeX - chunkX * 16;
  if(minLocalX < 0) {
   minLocalX = 0;
  }
  if(maxLocalX > 16) {
   maxLocalX = 16;
  }
  for(int chunkZ = chunkMinZ; chunkZ <= chunkMaxZ; ++chunkZ) {
   int minLocalZ = z - chunkZ * 16;
   int maxLocalZ = z + sizeZ - chunkZ * 16;
   if(minLocalZ < 0) {
    minLocalZ = 0;
   }
   if(maxLocalZ > 16) {
    maxLocalZ = 16;
   }
   offset = getChunk(chunkX, chunkZ)
                .toPacket(bytes, minLocalX, minY, minLocalZ, maxLocalX, maxY, maxLocalZ, offset);
  }
 }
 return bytes;
}
ChunkSource* World::createChunkCache() {
 if(dimension == nullptr) {
  return getChunkSource();
 }
 std::unique_ptr<ChunkStorage> chunkStorage;
 if(dimensionData_ != nullptr) {
  chunkStorage = dimensionData_->getChunkStorage(dimension.get());
 } else {
  chunkStorage = std::make_unique<NullChunkStorage>();
 }
 chunkGeneratorSource_ = dimension->createChunkGenerator();
 auto cache = std::make_unique<world::chunk::ChunkCache>(this, std::move(chunkStorage), chunkGeneratorSource_.get());
 setChunkCache(std::move(cache));
 return getChunkSource();
}
void World::setChunkCacheCenter(int chunkX, int chunkZ) {
 if(chunkCache_ != nullptr) {
  chunkCache_->setChunkCacheCenter(chunkX, chunkZ);
 }
}
void World::setChunkCacheCenterFromBlockPos(int blockX, int blockZ) {
 setChunkCacheCenter(chunk_coord(blockX), chunk_coord(blockZ));
}
void World::populateChunkCacheReadyChunks() {
}
void World::pumpChunkPublish() {
 if(chunkCache_ != nullptr) {
  chunkCache_->pumpChunkPublish();
 }
}
bool World::isChunkDataReady(int chunkX, int chunkZ) const {
 return chunkCache_ == nullptr || chunkCache_->isChunkDataReady(chunkX, chunkZ);
}
bool World::isPosLoaded(int x, int y, int z) const {
  if(y < 0 || y >= Chunk::height) {
   return false;
  }
  return getChunkIfLoaded(x, z) != nullptr;
}
void World::handleChunkDataUpdate(
    int x, int y, int z, int sizeX, int sizeY, int sizeZ, const std::vector<std::uint8_t>& chunkData) {
 const int minChunkX = x >> 4;
 const int minChunkZ = z >> 4;
 const int maxChunkX = (x + sizeX - 1) >> 4;
 const int maxChunkZ = (z + sizeZ - 1) >> 4;
 int offset = 0;
 int minY = y;
 int maxY = y + sizeY;
 if(minY < 0) {
  minY = 0;
 }
 if(maxY > Chunk::height) {
  maxY = Chunk::height;
 }
 for(int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
  int localMinX = x - chunkX * 16;
  int localMaxX = x + sizeX - chunkX * 16;
  if(localMinX < 0) {
   localMinX = 0;
  }
  if(localMaxX > 16) {
   localMaxX = 16;
  }
  for(int chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
   int localMinZ = z - chunkZ * 16;
   int localMaxZ = z + sizeZ - chunkZ * 16;
   if(localMinZ < 0) {
    localMinZ = 0;
   }
   if(localMaxZ > 16) {
    localMaxZ = 16;
   }
   if(chunkCache_ != nullptr && !chunkCache_->isChunkLoaded(chunkX, chunkZ)) {
    chunkCache_->loadChunk(chunkX, chunkZ);
   }
   offset = getChunk(chunkX, chunkZ)
                .loadFromPacket(chunkData, localMinX, minY, localMinZ, localMaxX, maxY, localMaxZ, offset);
    if(chunkCache_ != nullptr) {
     chunkCache_->markChunkDataReady(chunkX, chunkZ);
    }
   setBlocksDirty(chunkX * 16 + localMinX,
                  minY,
                  chunkZ * 16 + localMinZ,
                  chunkX * 16 + localMaxX,
                  maxY,
                  chunkZ * 16 + localMaxZ);
  }
 }
}
block::entity::BlockEntity* World::getBlockEntity(int x, int y, int z) {
 return getChunk(chunk_coord(x), chunk_coord(z)).getBlockEntity(mod_16(x), y, mod_16(z));
}
int World::getTopSolidBlockY(int x, int z) const {
 const Chunk& chunk = getChunk(chunk_coord(x), chunk_coord(z));
 x = mod_16(x);
 z = mod_16(z);
 for(int y = Chunk::height - 1; y > 0; --y) {
  const int blockId = chunk.getBlockId(x, y, z);
  const Block* block =
      blockId > 0 && blockId < Block::BLOCK_COUNT ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
  if(block == nullptr) {
   continue;
  }
  const auto& material = block->material;
  if(!material.blocksMovement() && !material.isFluid()) {
   continue;
  }
  return y + 1;
 }
 return -1;
}
void World::tickChunks() {
 while(chunkCache_ != nullptr && chunkCache_->tick()) {
 }
}
bool World::isTopY(int x, int y, int z) const {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return false;
 }
 if(y < 0) {
  return false;
 }
 if(y >= Chunk::height) {
  return true;
 }
 if(!hasChunk(x >> 4, z >> 4)) {
  return false;
 }
 const Chunk& chunk = getChunk(x >> 4, z >> 4);
 return chunk.isAboveMaxHeight(mod_16(x), y, mod_16(z));
}
void World::manageChunkUpdatesAndEvents() {
 activeChunks_.clear();
 for(PlayerEntity* player : players) {
  if(player == nullptr) {
   continue;
  }
  const int chunkX = MathHelper::floor(player->x / 16.0);
  const int chunkZ = MathHelper::floor(player->z / 16.0);
  constexpr int radius = 9;
  for(int dx = -radius; dx <= radius; ++dx) {
   for(int dz = -radius; dz <= radius; ++dz) {
    activeChunks_.insert(ChunkPos{dx + chunkX, dz + chunkZ});
   }
  }
 }
 if(ambientSoundCounter_ > 0) {
  --ambientSoundCounter_;
 }
 for(const ChunkPos& chunkPos : activeChunks_) {
  if(chunkCache_ != nullptr && !chunkCache_->isChunkLoaded(chunkPos.x, chunkPos.z)) {
   continue;
  }
  const int chunkOriginX = chunkPos.x * 16;
  const int chunkOriginZ = chunkPos.z * 16;
  Chunk& chunk = getChunk(chunkPos.x, chunkPos.z);
  if(ambientSoundCounter_ == 0) {
   lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
   const int randomValue = lcgBlockSeed_ >> 2;
   const int localX = randomValue & 0xF;
   const int localZ = (randomValue >> 8) & 0xF;
   const int localY = (randomValue >> 16) & 0x7F;
   const int blockId = chunk.getBlockId(localX, localY, localZ);
   const int worldX = localX + chunkOriginX;
   const int worldZ = localZ + chunkOriginZ;
   if(blockId == 0 && getBrightness(worldX, localY, worldZ) <= random_.nextInt(8) &&
      getBrightness(LightType::Sky, worldX, localY, worldZ) <= 0) {
    if(PlayerEntity* closest = getClosestPlayer(static_cast<double>(worldX) + 0.5,
                                                static_cast<double>(localY) + 0.5,
                                                static_cast<double>(worldZ) + 0.5,
                                                8.0)) {
     if(closest->getSquaredDistance(static_cast<double>(worldX) + 0.5,
                                    static_cast<double>(localY) + 0.5,
                                    static_cast<double>(worldZ) + 0.5) > 4.0) {
      playSound(static_cast<double>(worldX) + 0.5,
                static_cast<double>(localY) + 0.5,
                static_cast<double>(worldZ) + 0.5,
                "ambient.cave.cave",
                0.7f,
                0.8f + random_.nextFloat() * 0.2f);
      ambientSoundCounter_ = random_.nextInt(12000) + 6000;
     }
    }
   }
  }
  if(random_.nextInt(100000) == 0 && isRaining() && isThundering()) {
   lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
   const int randomValue = lcgBlockSeed_ >> 2;
   const int worldX = chunkOriginX + (randomValue & 0xF);
   const int worldZ = chunkOriginZ + ((randomValue >> 8) & 0xF);
   const int worldY = getTopSolidBlockY(worldX, worldZ);
   if(isRaining(worldX, worldY, worldZ)) {
    mod::LightningStrikeEvent event{this, worldX, worldY, worldZ, false};
    if(!event.canceled) {
     spawnGlobalEntity(new LightningEntity(this, event.x, event.y, event.z));
     weather_.ticksSinceLightning = 2;
    }
   }
  }
  if(random_.nextInt(16) == 0) {
   lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
   const int randomValue = lcgBlockSeed_ >> 2;
   const int localX = randomValue & 0xF;
   const int localZ = (randomValue >> 8) & 0xF;
   const int worldX = localX + chunkOriginX;
   const int worldZ = localZ + chunkOriginZ;
   const int worldY = getTopSolidBlockY(worldX, worldZ);
   if(getBiome(worldX, worldZ).canSnow() && worldY >= 0 && worldY < Chunk::height &&
      chunk.getLight(LightType::Block, localX, worldY, localZ) < 10) {
    const int belowId = chunk.getBlockId(localX, worldY - 1, localZ);
    const int currentId = chunk.getBlockId(localX, worldY, localZ);
    if(isRaining() && currentId == 0 && Block::SNOW != nullptr &&
       Block::SNOW->canPlaceAt(this, worldX, worldY, worldZ) && belowId != 0 &&
       Block::BLOCKS[static_cast<std::size_t>(belowId)] != nullptr &&
       Block::BLOCKS[static_cast<std::size_t>(belowId)]->material.blocksMovement()) {
     setBlock(worldX, worldY, worldZ, Block::SNOW->id);
    }
    if(Block::WATER != nullptr && belowId == Block::WATER->id &&
       chunk.getBlockMeta(localX, worldY - 1, localZ) == 0) {
     setBlock(worldX, worldY - 1, worldZ, Block::ICE->id);
    }
   }
  }
  for(int tickIndex = 0; tickIndex < 80; ++tickIndex) {
   lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
   const int randomValue = lcgBlockSeed_ >> 2;
   const int localX = randomValue & 0xF;
   const int localZ = (randomValue >> 8) & 0xF;
   const int localY = (randomValue >> 16) & 0x7F;
   const int blockId = chunk.getBlockId(localX, localY, localZ);
   if(blockId <= 0 || blockId >= Block::BLOCK_COUNT ||
      !Block::BLOCKS_RANDOM_TICK[static_cast<std::size_t>(blockId)]) {
    continue;
   }
   Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
   if(block != nullptr) {
    mod::RandomBlockTickEvent event{
        this, block, localX + chunkOriginX, localY, localZ + chunkOriginZ, blockId, false};
    if(!event.canceled) {
     block->onTick(this, event.x, event.y, event.z, random_);
    }
   }
  }
 }
}
} // namespace net::minecraft
