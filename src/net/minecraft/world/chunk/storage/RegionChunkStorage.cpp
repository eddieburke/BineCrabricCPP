#include "net/minecraft/world/chunk/storage/RegionChunkStorage.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkNbtCodec.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
namespace net::minecraft {
RegionChunkStorage::RegionChunkStorage(fs::path dir) : dir_(std::move(dir)) {
}
Chunk RegionChunkStorage::loadChunk(World* world, int chunkX, int chunkZ) {
 const std::optional<std::vector<std::uint8_t>> raw = RegionIo::readChunkData(dir_, chunkX, chunkZ);
 if(!raw.has_value()) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
 try {
  Nbt root = Nbt::read(*raw);
  NbtCompound rootCompound = NbtCompound::bind(root);
  return AlphaChunkStorage::loadChunkFromRootNbt(world, rootCompound, chunkX, chunkZ);
 } catch(const std::exception&) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
}
void RegionChunkStorage::saveChunk(World* world, Chunk& chunk) {
 if(world == nullptr) {
  return;
 }
 world->checkSessionLock();
 try {
  std::vector<std::uint8_t> raw;
  AlphaChunkNbtCodec::writeRootChunk(raw, chunk, world);
  RegionIo::writeChunkData(dir_, chunk.x, chunk.z, raw);
  WorldProperties& properties = world->getProperties();
  properties.setSizeOnDisk(properties.getSizeOnDisk() +
                           static_cast<std::uint64_t>(RegionIo::getChunkSize(dir_, chunk.x, chunk.z)));
 } catch(const std::exception&) {
 }
}
void RegionChunkStorage::saveEntities(World* world, Chunk& chunk) {
 (void)world;
 (void)chunk;
}
void RegionChunkStorage::tick() {
}
void RegionChunkStorage::flush() {
 RegionIo::sync();
}
} // namespace net::minecraft
