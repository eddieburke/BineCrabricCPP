#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
namespace net::minecraft::mod::lua {
namespace {
struct ActiveChunkState {
  Chunk* chunk = nullptr;
  World* world = nullptr;
  int chunkX = 0;
  int chunkZ = 0;
  ChunkWriteMode mode = ChunkWriteMode::ChunkApi;
  int depth = 0;
};
ActiveChunkState& activeState() {
  thread_local ActiveChunkState value;
  return value;
}
[[nodiscard]] bool inBounds(int localX, int y, int localZ) noexcept {
  return localX >= 0 && localX < Chunk::width && localZ >= 0 && localZ < Chunk::depth && y >= 0 && y < Chunk::height;
}
} // namespace
LuaChunkContext::Scope::Scope(Chunk* chunk, World* world, int chunkX, int chunkZ, ChunkWriteMode mode) {
  ActiveChunkState& state = activeState();
  if(state.depth == 0) {
    state.chunk = chunk;
    state.world = world;
    state.chunkX = chunkX;
    state.chunkZ = chunkZ;
    state.mode = mode;
  }
  ++state.depth;
}
LuaChunkContext::Scope::~Scope() {
  ActiveChunkState& state = activeState();
  if(state.depth <= 0) {
    return;
  }
  --state.depth;
  if(state.depth == 0) {
    state.chunk = nullptr;
    state.world = nullptr;
    state.chunkX = 0;
    state.chunkZ = 0;
    state.mode = ChunkWriteMode::ChunkApi;
  }
}
bool LuaChunkContext::hasActiveChunk() noexcept {
  return activeState().depth > 0 && activeState().chunk != nullptr;
}
Chunk* LuaChunkContext::activeChunk() noexcept {
  return activeState().chunk;
}
World* LuaChunkContext::activeWorld() noexcept {
  return activeState().world;
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
    return true;
  }
  return chunk->setBlock(localX, y, localZ, blockId);
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
  if(chunk == nullptr || localX < 0 || localX >= Chunk::width || localZ < 0 || localZ >= Chunk::depth) {
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
} // namespace net::minecraft::mod::lua
