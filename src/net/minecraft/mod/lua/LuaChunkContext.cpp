#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
namespace net::minecraft::mod::lua {
namespace {
struct ActiveChunkState {
 Chunk* chunk = nullptr;
 World* world = nullptr;
 BiomeSource* biomeSource = nullptr;
 int chunkX = 0;
 int chunkZ = 0;
 ChunkWriteMode mode = ChunkWriteMode::ChunkApi;
};
ActiveChunkState& activeState() {
 thread_local ActiveChunkState value;
 return value;
}
[[nodiscard]] bool inBounds(int localX, int y, int localZ) noexcept {
 return localX >= 0 && localX < Chunk::width && localZ >= 0 && localZ < Chunk::depth && y >= 0 && y < Chunk::height;
}
[[nodiscard]] bool inColumnBounds(int localX, int localZ) noexcept {
 return localX >= 0 && localX < Chunk::width && localZ >= 0 && localZ < Chunk::depth;
}
} // namespace
LuaChunkContext::Scope::Scope(Chunk* chunk,
                              World* world,
                              int chunkX,
                              int chunkZ,
                              ChunkWriteMode mode,
                              BiomeSource* biomeSource) {
 ActiveChunkState& state = activeState();
 previousChunk_ = state.chunk;
 previousWorld_ = state.world;
 previousBiomeSource_ = state.biomeSource;
 previousChunkX_ = state.chunkX;
 previousChunkZ_ = state.chunkZ;
 previousMode_ = state.mode;
 state = {chunk, world, biomeSource, chunkX, chunkZ, mode};
}
LuaChunkContext::Scope::~Scope() {
 ActiveChunkState& state = activeState();
 state = {previousChunk_, previousWorld_, previousBiomeSource_, previousChunkX_, previousChunkZ_, previousMode_};
}
bool LuaChunkContext::hasActiveChunk() noexcept {
 return activeState().chunk != nullptr;
}
Chunk* LuaChunkContext::activeChunk() noexcept {
 return activeState().chunk;
}
World* LuaChunkContext::activeWorld() noexcept {
 return activeState().world;
}
BiomeSource* LuaChunkContext::activeBiomeSource() noexcept {
 if(BiomeSource* source = activeState().biomeSource; source != nullptr) {
  return source;
 }
 if(World* world = activeState().world; world != nullptr) {
  return world->getBiomeSource();
 }
 return nullptr;
}
int LuaChunkContext::activeChunkX() noexcept {
 return activeState().chunkX;
}
int LuaChunkContext::activeChunkZ() noexcept {
 return activeState().chunkZ;
}
ChunkWriteMode LuaChunkContext::writeMode() noexcept {
 return activeState().mode;
}
bool LuaChunkContext::setBlock(int localX, int y, int localZ, int blockId) {
 Chunk* chunk = activeChunk();
 if(chunk == nullptr || !inBounds(localX, y, localZ)) {
  return false;
 }
 if(writeMode() == ChunkWriteMode::RawGeneration) {
  Generator::setRawBlock(*chunk, localX, y, localZ, blockId);
  chunk->dirty = true;
  runtime::WorldRequiredMods::notePlaced(activeWorld(), blockId);
  return true;
 }
 return chunk->setBlock(localX, y, localZ, blockId);
}
bool LuaChunkContext::fillDensity(const double* densityGrid, std::size_t count, int seaLevel) {
 Chunk* chunk = activeChunk();
 static constexpr int cellSize = 4;
 static constexpr int sizeX = cellSize + 1;
 static constexpr int sizeY = 17;
 static constexpr int sizeZ = cellSize + 1;
 static constexpr std::size_t expectedCount = static_cast<std::size_t>(sizeX * sizeY * sizeZ);
 if(chunk == nullptr || densityGrid == nullptr || count < expectedCount) {
  return false;
 }
 seaLevel = std::clamp(seaLevel, 0, Chunk::height);
 const int stoneId = Block::STONE != nullptr ? Block::STONE->id : 1;
 const int waterId = Block::WATER != nullptr ? Block::WATER->id : 9;
 const int iceId = Block::ICE != nullptr ? Block::ICE->id : 79;
 for(int cellX = 0; cellX < cellSize; ++cellX) {
  for(int cellZ = 0; cellZ < cellSize; ++cellZ) {
   for(int cellY = 0; cellY < 16; ++cellY) {
    double density00 =
        densityGrid[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
    double density01 =
        densityGrid[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
    double density10 =
        densityGrid[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
    double density11 =
        densityGrid[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
    const double dy0 =
        (densityGrid[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density00) *
        0.125;
    const double dy1 =
        (densityGrid[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density01) *
        0.125;
    const double dy2 =
        (densityGrid[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density10) *
        0.125;
    const double dy3 =
        (densityGrid[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density11) *
        0.125;
    for(int subY = 0; subY < 8; ++subY) {
     double x0 = density00;
     double x1 = density01;
     const double dx0 = (density10 - density00) * 0.25;
     const double dx1 = (density11 - density01) * 0.25;
     for(int subX = 0; subX < 4; ++subX) {
      double density = x0;
      const double dz = (x1 - x0) * 0.25;
      for(int subZ = 0; subZ < 4; ++subZ) {
       const int localX = cellX * 4 + subX;
       const int localY = cellY * 8 + subY;
       const int localZ = cellZ * 4 + subZ;
       const double temperature = getTemperature(localX, localZ);
       int blockId = 0;
       if(localY < seaLevel) {
        blockId = temperature < 0.5 && localY >= seaLevel - 1 ? iceId : waterId;
       }
       if(density > 0.0) {
        blockId = stoneId;
       }
       setBlock(localX, localY, localZ, blockId);
       density += dz;
      }
      x0 += dx0;
      x1 += dx1;
     }
     density00 += dy0;
     density01 += dy1;
     density10 += dy2;
     density11 += dy3;
    }
   }
  }
 }
 return true;
}
int LuaChunkContext::getBlock(int localX, int y, int localZ) {
 const Chunk* chunk = activeChunk();
 if(chunk == nullptr || !inBounds(localX, y, localZ)) {
  return 0;
 }
 return Generator::rawBlock(*chunk, localX, y, localZ);
}
int LuaChunkContext::getHeight(int localX, int localZ) {
 const Chunk* chunk = activeChunk();
 if(chunk == nullptr || !inColumnBounds(localX, localZ)) {
  return 0;
 }
 if(writeMode() == ChunkWriteMode::RawGeneration) {
  for(int y = Chunk::height - 1; y >= 0; --y) {
   const int blockId = Generator::rawBlock(*chunk, localX, y, localZ);
   if(Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId & 0xFF)] != 0) {
    return y + 1;
   }
  }
  return 0;
 }
 return chunk->getHeight(localX, localZ);
}
int LuaChunkContext::getTopSolidY(int localX, int localZ) {
 const Chunk* chunk = activeChunk();
 if(chunk == nullptr || !inColumnBounds(localX, localZ)) {
  return -1;
 }
 const int waterId = Block::WATER != nullptr ? Block::WATER->id : 9;
 const int flowingWaterId = Block::FLOWING_WATER != nullptr ? Block::FLOWING_WATER->id : 8;
 const int iceId = Block::ICE != nullptr ? Block::ICE->id : 79;
 for(int y = Chunk::height - 1; y >= 0; --y) {
  const int blockId = Generator::rawBlock(*chunk, localX, y, localZ);
  if(blockId != 0 && blockId != waterId && blockId != flowingWaterId && blockId != iceId) {
   return y;
  }
 }
 return -1;
}
double LuaChunkContext::getTemperature(int localX, int localZ) {
 if(!inColumnBounds(localX, localZ)) {
  return 0.5;
 }
 if(BiomeSource* source = activeBiomeSource(); source != nullptr) {
  if(source->temperatureMap().size() == static_cast<std::size_t>(Chunk::width * Chunk::depth)) {
   return source->temperatureMap()[static_cast<std::size_t>(localX * Chunk::depth + localZ)];
  }
  return source->sampleClimate(activeChunkX() * Chunk::width + localX, activeChunkZ() * Chunk::depth + localZ)
      .temperature;
 }
 return 0.5;
}
double LuaChunkContext::getDownfall(int localX, int localZ) {
 if(!inColumnBounds(localX, localZ)) {
  return 0.5;
 }
 if(BiomeSource* source = activeBiomeSource(); source != nullptr) {
  if(source->downfallMap().size() == static_cast<std::size_t>(Chunk::width * Chunk::depth)) {
   return source->downfallMap()[static_cast<std::size_t>(localX * Chunk::depth + localZ)];
  }
  return source->sampleClimate(activeChunkX() * Chunk::width + localX, activeChunkZ() * Chunk::depth + localZ)
      .downfall;
 }
 return 0.5;
}
std::string_view LuaChunkContext::getBiomeName(int localX, int localZ) {
 Biome::init();
 return Biome::getBiome(getTemperature(localX, localZ), getDownfall(localX, localZ)).wireName();
}
} // namespace net::minecraft::mod::lua
